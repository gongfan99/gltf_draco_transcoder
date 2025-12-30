// Copyright 2025 The Draco Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "selective_draco_transcoder.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <set>
#include <cstring>
#include <cstdlib>
#include <sstream>

#include "draco/core/status.h"

namespace selective_draco
{

    template <typename... Args>
    inline void Log(Args &&...args)
    {
        (std::cout << ... << args) << std::endl;
    }

    // Helper to get buffer data from tinygltf accessor
    template <typename T>
    std::vector<T> GetAccessorData(const tinygltf::Model &model, int accessor_index)
    {
        const auto &accessor = model.accessors[accessor_index];
        const auto &buffer_view = model.bufferViews[accessor.bufferView];
        const auto &buffer = model.buffers[buffer_view.buffer];

        size_t element_size = tinygltf::GetComponentSizeInBytes(accessor.componentType);
        size_t component_count = tinygltf::GetNumComponentsInType(accessor.type);
        size_t stride = buffer_view.byteStride ? buffer_view.byteStride : element_size * component_count;

        std::vector<T> data(accessor.count * component_count);
        const uint8_t *buffer_data = buffer.data.data() + buffer_view.byteOffset + accessor.byteOffset;

        for (size_t i = 0; i < accessor.count; ++i)
        {
            for (size_t j = 0; j < component_count; ++j)
            {
                size_t offset = i * stride + j * element_size;
                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    float value;
                    memcpy(&value, buffer_data + offset, sizeof(float));
                    data[i * component_count + j] = static_cast<T>(value);
                }
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                {
                    unsigned int value;
                    memcpy(&value, buffer_data + offset, sizeof(unsigned int));
                    data[i * component_count + j] = static_cast<T>(value);
                }
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    unsigned short value;
                    memcpy(&value, buffer_data + offset, sizeof(unsigned short));
                    data[i * component_count + j] = static_cast<T>(value);
                }
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                {
                    unsigned char value;
                    memcpy(&value, buffer_data + offset, sizeof(unsigned char));
                    data[i * component_count + j] = static_cast<T>(value);
                }
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_BYTE)
                {
                    char value;
                    memcpy(&value, buffer_data + offset, sizeof(char));
                    data[i * component_count + j] = static_cast<T>(value);
                }
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT)
                {
                    short value;
                    memcpy(&value, buffer_data + offset, sizeof(short));
                    data[i * component_count + j] = static_cast<T>(value);
                }
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_INT)
                {
                    int value;
                    memcpy(&value, buffer_data + offset, sizeof(int));
                    data[i * component_count + j] = static_cast<T>(value);
                }
                // Add more types as needed
            }
        }
        return data;
    }

    // Encode a primitive as Draco (for TRIANGLES/POINTS only)
    draco::StatusOr<std::vector<uint8_t>> EncodePrimitiveToDraco(const tinygltf::Model &model,
                                                                 const tinygltf::Primitive &primitive,
                                                                 const DracoOptions &options)
    {
        Log("    Creating Draco mesh...");
        draco::Mesh mesh;

        // Get vertex count from POSITION attribute first (needed for index validation)
        if (primitive.attributes.find("POSITION") == primitive.attributes.end())
        {
            return draco::Status(draco::Status::DRACO_ERROR, "No POSITION attribute found");
        }
        size_t vertex_count = model.accessors[primitive.attributes.at("POSITION")].count;
        mesh.set_num_points(vertex_count);

        // Handle indices
        if (primitive.indices >= 0)
        {
            const auto &indices_accessor = model.accessors[primitive.indices];
            Log("    Processing indices, accessor type: ", indices_accessor.componentType);
            if (indices_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                auto indices = GetAccessorData<uint32_t>(model, primitive.indices);
                Log("    Got ", indices.size(), " UNSIGNED_INT indices");
                uint32_t max_idx = 0;
                for (auto idx : indices)
                {
                    if (idx > max_idx)
                        max_idx = idx;
                }
                Log("    Max index value: ", max_idx);
                if (max_idx >= vertex_count)
                {
                    return draco::Status(draco::Status::DRACO_ERROR, "Index out of range");
                }
                draco::Mesh::Face face;
                for (size_t i = 0; i < indices.size(); i += 3)
                {
                    face[0] = indices[i];
                    face[1] = indices[i + 1];
                    face[2] = indices[i + 2];
                    mesh.AddFace(face);
                }
                Log("    Added ", indices.size() / 3, " faces");
            }
            else if (indices_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                auto indices = GetAccessorData<uint16_t>(model, primitive.indices);
                Log("    Got ", indices.size(), " UNSIGNED_SHORT indices");
                uint32_t max_idx = 0;
                for (auto idx : indices)
                {
                    if (static_cast<uint32_t>(idx) > max_idx)
                        max_idx = static_cast<uint32_t>(idx);
                }
                Log("    Max index value: ", max_idx, " (vertex count: ", vertex_count, ")");

                // Validate index range to prevent out-of-bounds access
                if (max_idx >= vertex_count)
                {
                    return draco::Status(draco::Status::DRACO_ERROR, "Index out of range");
                }

                draco::Mesh::Face face;
                for (size_t i = 0; i < indices.size(); i += 3)
                {
                    face[0] = static_cast<uint32_t>(indices[i]);
                    face[1] = static_cast<uint32_t>(indices[i + 1]);
                    face[2] = static_cast<uint32_t>(indices[i + 2]);
                    mesh.AddFace(face);
                }
                Log("    Added ", indices.size() / 3, " faces");
            }
            else if (indices_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                auto indices = GetAccessorData<uint8_t>(model, primitive.indices);
                Log("    Got ", indices.size(), " UNSIGNED_BYTE indices");
                uint32_t max_idx = 0;
                for (auto idx : indices)
                {
                    if (static_cast<uint32_t>(idx) > max_idx)
                        max_idx = static_cast<uint32_t>(idx);
                }
                Log("    Max index value: ", max_idx);
                if (max_idx >= vertex_count)
                {
                    return draco::Status(draco::Status::DRACO_ERROR, "Index out of range");
                }
                draco::Mesh::Face face;
                for (size_t i = 0; i < indices.size(); i += 3)
                {
                    face[0] = static_cast<uint32_t>(indices[i]);
                    face[1] = static_cast<uint32_t>(indices[i + 1]);
                    face[2] = static_cast<uint32_t>(indices[i + 2]);
                    mesh.AddFace(face);
                }
                Log("    Added ", indices.size() / 3, " faces");
            }
            else
            {
                return draco::Status(draco::Status::DRACO_ERROR, "Unsupported index type");
            }
        }
        else
        {
            Log("    No indices, will generate sequentially");
        }

        // Add attributes - support all standard glTF attributes
        int draco_attr_id = 0;

        // POSITION
        if (primitive.attributes.find("POSITION") != primitive.attributes.end())
        {
            Log("    Adding POSITION attribute...");
            auto positions = GetAccessorData<float>(model, primitive.attributes.at("POSITION"));
            Log("    Got ", positions.size(), " position floats (", positions.size() / 3, " vertices)");
            draco::GeometryAttribute pos_attr;
            pos_attr.Init(draco::GeometryAttribute::POSITION, nullptr, 3, draco::DT_FLOAT32,
                          false, 3 * sizeof(float), 0);
            int pos_att_id = mesh.AddAttribute(pos_attr, true, positions.size() / 3);
            Log("    Adding position values...");
            for (size_t i = 0; i < positions.size() / 3; ++i)
            {
                mesh.attribute(pos_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &positions[i * 3]);
            }
            Log("    POSITION attribute added");
            draco_attr_id++;
        }

        // NORMAL
        if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
        {
            Log("    Adding NORMAL attribute...");
            auto normals = GetAccessorData<float>(model, primitive.attributes.at("NORMAL"));
            Log("    Got ", normals.size(), " normal floats (", normals.size() / 3, " vertices)");
            draco::GeometryAttribute normal_attr;
            normal_attr.Init(draco::GeometryAttribute::NORMAL, nullptr, 3, draco::DT_FLOAT32,
                             false, 3 * sizeof(float), 0);
            int normal_att_id = mesh.AddAttribute(normal_attr, true, normals.size() / 3);
            Log("    Adding normal values...");
            for (size_t i = 0; i < normals.size() / 3; ++i)
            {
                mesh.attribute(normal_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &normals[i * 3]);
            }
            Log("    NORMAL attribute added");
            draco_attr_id++;
        }

        // TANGENT
        if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
        {
            Log("    Adding TANGENT attribute...");
            auto tangents = GetAccessorData<float>(model, primitive.attributes.at("TANGENT"));
            Log("    Got ", tangents.size(), " tangent floats (", tangents.size() / 4, " vertices)");
            draco::GeometryAttribute tangent_attr;
            tangent_attr.Init(draco::GeometryAttribute::TANGENT, nullptr, 4, draco::DT_FLOAT32,
                              false, 4 * sizeof(float), 0);
            int tangent_att_id = mesh.AddAttribute(tangent_attr, true, tangents.size() / 4);
            Log("    Adding tangent values...");
            for (size_t i = 0; i < tangents.size() / 4; ++i)
            {
                mesh.attribute(tangent_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &tangents[i * 4]);
            }
            Log("    TANGENT attribute added");
            draco_attr_id++;
        }

        // TEXCOORD_0
        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
        {
            Log("    Adding TEXCOORD_0 attribute...");
            auto texcoords = GetAccessorData<float>(model, primitive.attributes.at("TEXCOORD_0"));
            Log("    Got ", texcoords.size(), " texcoord floats (", texcoords.size() / 2, " vertices)");
            draco::GeometryAttribute texcoord_attr;
            texcoord_attr.Init(draco::GeometryAttribute::TEX_COORD, nullptr, 2, draco::DT_FLOAT32,
                               false, 2 * sizeof(float), 0);
            int texcoord_att_id = mesh.AddAttribute(texcoord_attr, true, texcoords.size() / 2);
            Log("    Adding texcoord values...");
            for (size_t i = 0; i < texcoords.size() / 2; ++i)
            {
                mesh.attribute(texcoord_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &texcoords[i * 2]);
            }
            Log("    TEXCOORD_0 attribute added");
            draco_attr_id++;
        }

        // TEXCOORD_1
        if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
        {
            Log("    Adding TEXCOORD_1 attribute...");
            auto texcoords = GetAccessorData<float>(model, primitive.attributes.at("TEXCOORD_1"));
            Log("    Got ", texcoords.size(), " texcoord floats (", texcoords.size() / 2, " vertices)");
            draco::GeometryAttribute texcoord_attr;
            texcoord_attr.Init(draco::GeometryAttribute::TEX_COORD, nullptr, 2, draco::DT_FLOAT32,
                               false, 2 * sizeof(float), 0);
            int texcoord_att_id = mesh.AddAttribute(texcoord_attr, true, texcoords.size() / 2);
            Log("    Adding texcoord values...");
            for (size_t i = 0; i < texcoords.size() / 2; ++i)
            {
                mesh.attribute(texcoord_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &texcoords[i * 2]);
            }
            Log("    TEXCOORD_1 attribute added");
            draco_attr_id++;
        }

        // COLOR_0
        if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
        {
            Log("    Adding COLOR_0 attribute...");
            const auto &color_accessor = model.accessors[primitive.attributes.at("COLOR_0")];
            int component_count = tinygltf::GetNumComponentsInType(color_accessor.type);
            Log("    Color has ", component_count, " components, type: ", color_accessor.componentType);

            if (color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                auto colors = GetAccessorData<float>(model, primitive.attributes.at("COLOR_0"));
                Log("    Got ", colors.size(), " color floats (", colors.size() / component_count, " vertices)");
                draco::GeometryAttribute color_attr;
                color_attr.Init(draco::GeometryAttribute::COLOR, nullptr, component_count, draco::DT_FLOAT32,
                                false, component_count * sizeof(float), 0);
                int color_att_id = mesh.AddAttribute(color_attr, true, colors.size() / component_count);
                Log("    Adding color values...");
                for (size_t i = 0; i < colors.size() / component_count; ++i)
                {
                    mesh.attribute(color_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &colors[i * component_count]);
                }
                Log("    COLOR_0 attribute added");
            }
            else if (color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                auto colors = GetAccessorData<uint8_t>(model, primitive.attributes.at("COLOR_0"));
                Log("    Got ", colors.size(), " color bytes (", colors.size() / component_count, " vertices)");
                // Convert uint8_t to float for Draco
                std::vector<float> float_colors(colors.size());
                for (size_t i = 0; i < colors.size(); ++i)
                {
                    float_colors[i] = static_cast<float>(colors[i]) / 255.0f;
                }
                draco::GeometryAttribute color_attr;
                color_attr.Init(draco::GeometryAttribute::COLOR, nullptr, component_count, draco::DT_FLOAT32,
                                false, component_count * sizeof(float), 0);
                int color_att_id = mesh.AddAttribute(color_attr, true, float_colors.size() / component_count);
                Log("    Adding color values...");
                for (size_t i = 0; i < float_colors.size() / component_count; ++i)
                {
                    mesh.attribute(color_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &float_colors[i * component_count]);
                }
                Log("    COLOR_0 attribute added");
            }
            draco_attr_id++;
        }

        // JOINTS_0 and WEIGHTS_0 (for skinning)
        if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end() &&
            primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
        {
            Log("    Adding JOINTS_0 and WEIGHTS_0 attributes...");
            const auto &joints_accessor = model.accessors[primitive.attributes.at("JOINTS_0")];
            int joints_component_count = tinygltf::GetNumComponentsInType(joints_accessor.type);
            auto joints = GetAccessorData<uint16_t>(model, primitive.attributes.at("JOINTS_0"));
            auto weights = GetAccessorData<float>(model, primitive.attributes.at("WEIGHTS_0"));

            Log("    Got ", joints.size(), " joints (", joints.size() / joints_component_count, " vertices)");
            Log("    Got ", weights.size(), " weights (", weights.size() / joints_component_count, " vertices)");

            // Draco treats joints and weights as generic attributes
            draco::GeometryAttribute joints_attr;
            joints_attr.Init(draco::GeometryAttribute::GENERIC, nullptr, joints_component_count, draco::DT_UINT16,
                             false, joints_component_count * sizeof(uint16_t), 0);
            int joints_att_id = mesh.AddAttribute(joints_attr, true, joints.size() / joints_component_count);
            for (size_t i = 0; i < joints.size() / joints_component_count; ++i)
            {
                mesh.attribute(joints_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &joints[i * joints_component_count]);
            }

            draco::GeometryAttribute weights_attr;
            weights_attr.Init(draco::GeometryAttribute::GENERIC, nullptr, joints_component_count, draco::DT_FLOAT32,
                              false, joints_component_count * sizeof(float), 0);
            int weights_att_id = mesh.AddAttribute(weights_attr, true, weights.size() / joints_component_count);
            for (size_t i = 0; i < weights.size() / joints_component_count; ++i)
            {
                mesh.attribute(weights_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &weights[i * joints_component_count]);
            }
            Log("    JOINTS_0 and WEIGHTS_0 attributes added");
            draco_attr_id += 2;
        }

        Log("    Mesh created with ", mesh.num_faces(), " faces and ", mesh.num_points(), " points");

        // Encode
        Log("    Setting up Draco encoder...");
        draco::Encoder encoder;
        encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, options.quantization_position);
        encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, options.quantization_normal);
        encoder.SetAttributeQuantization(draco::GeometryAttribute::TANGENT, options.quantization_tangent);
        encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, options.quantization_tex_coord);
        encoder.SetAttributeQuantization(draco::GeometryAttribute::COLOR, options.quantization_color);
        encoder.SetAttributeQuantization(draco::GeometryAttribute::GENERIC, options.quantization_generic);
        int speed = 10 - options.compression_level; // Map compression level to speed (0=fast, 10=slow/best compression)
        encoder.SetSpeedOptions(speed, speed);
        Log("    Encoder configured, speed=", speed);

        Log("    Starting Draco encoding...");
        draco::EncoderBuffer buffer;
        const draco::Status status = encoder.EncodeMeshToBuffer(mesh, &buffer);
        if (!status.ok())
        {
            return draco::Status(draco::Status::DRACO_ERROR, "Draco encoding failed: " + std::string(status.error_msg()));
        }

        Log("    Draco encoding completed successfully");
        return std::vector<uint8_t>(buffer.data(), buffer.data() + buffer.size());
    }

    // Main selective compression function
    draco::StatusOr<std::vector<uint8_t>> SelectiveCompressGltf(tinygltf::Model model, const DracoOptions &options)
    {
        // First pass: Analyze accessor and bufferView usage to determine which primitives can be compressed
        Log("Analyzing accessor and bufferView usage...");
        std::unordered_map<int, std::set<std::pair<size_t, size_t>>> accessor_owners;
        std::unordered_map<int, std::set<std::pair<size_t, size_t>>> buffer_view_owners;
        std::unordered_map<std::pair<size_t, size_t>, bool> can_compress_primitive;

        // Special key to mark shared resources (images, skins, animations, etc.)
        std::pair<size_t, size_t> shared_key = {SIZE_MAX, SIZE_MAX};

        // Step 1: Mark shared bufferViews (images, skins, etc.)
        for (size_t img_idx = 0; img_idx < model.images.size(); ++img_idx)
        {
            const auto &image = model.images[img_idx];
            if (image.bufferView >= 0)
            {
                buffer_view_owners[image.bufferView].insert(shared_key);
            }
        }

        for (size_t skin_idx = 0; skin_idx < model.skins.size(); ++skin_idx)
        {
            const auto &skin = model.skins[skin_idx];
            // Add inverseBindMatrices accessor if present
            if (skin.inverseBindMatrices >= 0)
            {
                accessor_owners[skin.inverseBindMatrices].insert(shared_key);
                int bv_idx = model.accessors[skin.inverseBindMatrices].bufferView;
                if (bv_idx >= 0)
                {
                    buffer_view_owners[bv_idx].insert(shared_key);
                }
            }
        }

        // Mark animation and material bufferViews as shared
        for (size_t anim_idx = 0; anim_idx < model.animations.size(); ++anim_idx)
        {
            const auto &animation = model.animations[anim_idx];
            for (const auto &channel : animation.channels)
            {
                if (channel.target_path == "translation" || channel.target_path == "rotation" || channel.target_path == "scale")
                {
                    // Mark sampler input/output accessors as shared
                    if (animation.samplers[channel.sampler].input >= 0)
                    {
                        accessor_owners[animation.samplers[channel.sampler].input].insert(shared_key);
                        int bv_idx = model.accessors[animation.samplers[channel.sampler].input].bufferView;
                        if (bv_idx >= 0)
                        {
                            buffer_view_owners[bv_idx].insert(shared_key);
                        }
                    }
                    if (animation.samplers[channel.sampler].output >= 0)
                    {
                        accessor_owners[animation.samplers[channel.sampler].output].insert(shared_key);
                        int bv_idx = model.accessors[animation.samplers[channel.sampler].output].bufferView;
                        if (bv_idx >= 0)
                        {
                            buffer_view_owners[bv_idx].insert(shared_key);
                        }
                    }
                }
            }
        }

        for (size_t mesh_idx = 0; mesh_idx < model.meshes.size(); ++mesh_idx)
        {
            for (size_t prim_idx = 0; prim_idx < model.meshes[mesh_idx].primitives.size(); ++prim_idx)
            {
                const auto &primitive = model.meshes[mesh_idx].primitives[prim_idx];
                auto prim_key = std::make_pair(mesh_idx, prim_idx);

                // Collect ALL accessors for this primitive (including uncompressed ones)
                std::vector<int> all_accessors;
                if (primitive.indices >= 0)
                {
                    all_accessors.push_back(primitive.indices);
                }

                // Add all attributes from the primitive
                for (const auto &[attr_name, acc_idx] : primitive.attributes)
                {
                    all_accessors.push_back(acc_idx);
                }

                // Record ownership for all accessors and their bufferViews
                for (int acc_idx : all_accessors)
                {
                    if (acc_idx >= 0 && acc_idx < (int)model.accessors.size())
                    {
                        accessor_owners[acc_idx].insert(prim_key);

                        int bv_idx = model.accessors[acc_idx].bufferView;
                        if (bv_idx >= 0 && bv_idx < (int)model.bufferViews.size())
                        {
                            buffer_view_owners[bv_idx].insert(prim_key);
                        }
                    }
                }
            }
        }

        // Second pass: Mark primitives that have exclusive access to their accessors and bufferViews
        for (size_t mesh_idx = 0; mesh_idx < model.meshes.size(); ++mesh_idx)
        {
            for (size_t prim_idx = 0; prim_idx < model.meshes[mesh_idx].primitives.size(); ++prim_idx)
            {
                const auto &primitive = model.meshes[mesh_idx].primitives[prim_idx];
                auto prim_key = std::make_pair(mesh_idx, prim_idx);

                bool can_compress = true;

                // Collect ALL accessors for this primitive (indices + all attributes)
                std::vector<int> all_accessors;
                if (primitive.indices >= 0)
                {
                    all_accessors.push_back(primitive.indices);
                }
                // Add all attributes from the primitive
                for (const auto &[attr_name, acc_idx] : primitive.attributes)
                {
                    all_accessors.push_back(acc_idx);
                }

                // Check exclusivity for ALL accessors (bufferViews don't block compression)
                for (int acc_idx : all_accessors)
                {
                    if (acc_idx >= 0 && acc_idx < (int)model.accessors.size())
                    {
                        // Check accessor exclusivity (must be used by exactly one primitive, not shared)
                        const auto &acc_owners = accessor_owners[acc_idx];
                        if (acc_owners.size() != 1 || acc_owners.find(prim_key) == acc_owners.end() ||
                            acc_owners.count(shared_key) > 0)
                        {
                            can_compress = false;
                            break;
                        }
                    }
                    if (!can_compress)
                        break;
                }

                // Only allow compression for TRIANGLES/POINTS
                int mode = primitive.mode;
                if (mode != TINYGLTF_MODE_TRIANGLES && mode != TINYGLTF_MODE_POINTS)
                {
                    can_compress = false;
                }

                can_compress_primitive[prim_key] = can_compress;
                Log("  Primitive ", prim_idx, " in mesh ", mesh_idx, " can_compress: ", (can_compress ? "YES" : "NO"));
            }
        }

        // Third pass: Encode all compressible primitives FIRST (before any bufferView modifications)
        Log("Processing meshes and primitives...");
        std::unordered_map<std::pair<size_t, size_t>, std::vector<uint8_t>> draco_bitstreams;

        for (size_t mesh_idx = 0; mesh_idx < model.meshes.size(); ++mesh_idx)
        {
            auto &mesh = model.meshes[mesh_idx];
            Log("Processing mesh ", mesh_idx, " with ", mesh.primitives.size(), " primitives");

            for (size_t prim_idx = 0; prim_idx < mesh.primitives.size(); ++prim_idx)
            {
                auto &primitive = mesh.primitives[prim_idx];
                int mode = primitive.mode; // Default 4 = TRIANGLES

                Log("  Primitive ", prim_idx, " has mode ", mode);

                auto prim_key = std::make_pair(mesh_idx, prim_idx);
                if (can_compress_primitive[prim_key])
                {
                    Log("  Encoding primitive with Draco...");
                    // Encode with Draco
                    auto draco_result = EncodePrimitiveToDraco(model, primitive, options);

                    if (draco_result.ok())
                    {
                        Log("  Draco encoding successful, size: ", draco_result.value().size(), " bytes");
                        // Store the Draco bitstream for later use in consolidation
                        draco_bitstreams[prim_key] = std::move(draco_result).value();

                        Log("  Draco extension will be added in consolidation phase");
                    }
                    else
                    {
                        Log("  Draco encoding failed: ", draco_result.status().error_msg());
                    }
                }
                else
                {
                    // Leave LINE_STRIP, LINE_LOOP, etc. unchanged
                    Log("  Skipping primitive (shared data or unsupported mode)");
                }
            }
        }

        // Fourth pass: Consolidate all data into a single buffer (AFTER encoding is complete)
        Log("Consolidating all data into single buffer...");
        std::vector<uint8_t> consolidated_buffer;
        size_t current_offset = 0;

        // Helper function to append data with 4-byte alignment
        auto append_data = [&](const std::vector<uint8_t> &data)
        {
            // Pad to 4-byte alignment
            size_t padding = (4 - (consolidated_buffer.size() % 4)) % 4;
            consolidated_buffer.insert(consolidated_buffer.end(), padding, 0);
            current_offset += padding;

            // Append data
            size_t start_offset = consolidated_buffer.size();
            consolidated_buffer.insert(consolidated_buffer.end(), data.begin(), data.end());
            return start_offset;
        };

        // Create a map from old bufferView index to new bufferView index
        std::vector<size_t> new_buffer_view_indices(model.bufferViews.size(), SIZE_MAX);

        // Process all bufferViews (images, skins, animations, etc.)
        Log("Processing ", model.bufferViews.size(), " buffer views...");
        for (size_t bv_idx = 0; bv_idx < model.bufferViews.size(); ++bv_idx)
        {
            const auto &buffer_view = model.bufferViews[bv_idx];
            const auto &buffer = model.buffers[buffer_view.buffer];

            // Check if ALL owners of this bufferView are compressed primitives
            bool skip_this_view = false;
            const auto &owners = buffer_view_owners[bv_idx];
            if (!owners.empty())
            {
                // If any owner is the shared_key, don't skip (preserve shared resources)
                if (owners.count(shared_key) == 0)
                {
                    // Check if every owner is a compressed primitive
                    skip_this_view = true;
                    for (const auto &owner : owners)
                    {
                        if (can_compress_primitive.find(owner) == can_compress_primitive.end() ||
                            !can_compress_primitive[owner])
                        {
                            skip_this_view = false;
                            break;
                        }
                    }
                }
            }

            if (skip_this_view)
            {
                Log("  Skipping bufferView ", bv_idx, " (used by compressed primitive)");
                // Create a dummy bufferView pointing to offset 0 with length 0
                model.bufferViews[bv_idx].buffer = 0;
                model.bufferViews[bv_idx].byteOffset = 0;
                model.bufferViews[bv_idx].byteLength = 0;
                new_buffer_view_indices[bv_idx] = bv_idx;
                continue;
            }

            // Extract the data from this bufferView
            std::vector<uint8_t> data(
                buffer.data.begin() + buffer_view.byteOffset,
                buffer.data.begin() + buffer_view.byteOffset + buffer_view.byteLength);

            size_t new_offset = append_data(data);

            // Update the bufferView to point to our consolidated buffer
            model.bufferViews[bv_idx].buffer = 0; // Will be buffer index 0
            model.bufferViews[bv_idx].byteOffset = new_offset;

            new_buffer_view_indices[bv_idx] = bv_idx;
        }

        // Now add the Draco extensions for compressed primitives
        for (size_t mesh_idx = 0; mesh_idx < model.meshes.size(); ++mesh_idx)
        {
            auto &mesh = model.meshes[mesh_idx];
            for (size_t prim_idx = 0; prim_idx < mesh.primitives.size(); ++prim_idx)
            {
                auto &primitive = mesh.primitives[prim_idx];
                auto prim_key = std::make_pair(mesh_idx, prim_idx);

                if (draco_bitstreams.find(prim_key) != draco_bitstreams.end())
                {
                    const auto &draco_data = draco_bitstreams[prim_key];
                    Log("  Adding Draco extension for primitive ", prim_idx, " in mesh ", mesh_idx);

                    // Append Draco data to consolidated buffer
                    size_t draco_offset = append_data(draco_data);

                    // Create buffer view for Draco data
                    tinygltf::BufferView draco_view;
                    draco_view.buffer = 0; // Consolidated buffer index
                    draco_view.byteOffset = draco_offset;
                    draco_view.byteLength = draco_data.size();
                    model.bufferViews.push_back(std::move(draco_view));

                    // Add KHR_draco_mesh_compression extension
                    tinygltf::Value::Object draco_ext;
                    draco_ext["bufferView"] = tinygltf::Value(int(model.bufferViews.size() - 1));

                    tinygltf::Value::Object attr_map;
                    // Map attributes to Draco attribute IDs (POSITION=0, NORMAL=1, TEXCOORD_0=2, COLOR_0=3, etc.)
                    int draco_attr_id = 0;
                    if (primitive.attributes.find("POSITION") != primitive.attributes.end())
                    {
                        attr_map["POSITION"] = tinygltf::Value(draco_attr_id++);
                    }
                    if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
                    {
                        attr_map["NORMAL"] = tinygltf::Value(draco_attr_id++);
                    }
                    if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
                    {
                        attr_map["TANGENT"] = tinygltf::Value(draco_attr_id++);
                    }
                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                    {
                        attr_map["TEXCOORD_0"] = tinygltf::Value(draco_attr_id++);
                    }
                    if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
                    {
                        attr_map["TEXCOORD_1"] = tinygltf::Value(draco_attr_id++);
                    }
                    if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
                    {
                        attr_map["COLOR_0"] = tinygltf::Value(draco_attr_id++);
                    }
                    if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
                    {
                        attr_map["JOINTS_0"] = tinygltf::Value(draco_attr_id++);
                        attr_map["WEIGHTS_0"] = tinygltf::Value(draco_attr_id++);
                    }

                    draco_ext["attributes"] = tinygltf::Value(attr_map);

                    primitive.extensions["KHR_draco_mesh_compression"] = tinygltf::Value(draco_ext);

                    Log("  Draco extension added to primitive");
                }
            }
        }

        // Replace buffers with our single consolidated buffer
        model.buffers.clear();
        tinygltf::Buffer consolidated_gltf_buffer;
        consolidated_gltf_buffer.data = std::move(consolidated_buffer);
        model.buffers.push_back(std::move(consolidated_gltf_buffer));

        // Set asset information
        model.asset.version = "2.0";
        model.asset.generator = "gltf_draco_transcoder";

        // Add extensions
        model.extensionsUsed.push_back("KHR_draco_mesh_compression");
        model.extensionsRequired.push_back("KHR_draco_mesh_compression");

        // Write back to glTF binary
        tinygltf::TinyGLTF writer;
        std::stringstream output_stream;
        if (!writer.WriteGltfSceneToStream(&model, output_stream, false, true))
        {
            return draco::Status(draco::Status::DRACO_ERROR, "Failed to write glTF");
        }

        std::string output = output_stream.str();
        return std::vector<uint8_t>(output.begin(), output.end());
    }

} // namespace selective_draco

extern "C"
{

    void *draco_transcode_gltf_from_buffer(const void *input_data,
                                           size_t input_size, const DracoOptions *options,
                                           size_t *output_size)
    {
        if (!input_data || !input_size || !options || !output_size)
        {
            return nullptr;
        }

        *output_size = 0;

        // Load glTF from buffer
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string err;
        std::string warn;

        // Load GLB from memory
        std::string input_str(reinterpret_cast<const char *>(input_data), input_size);
        if (!loader.LoadBinaryFromMemory(&model, &err, &warn, reinterpret_cast<const unsigned char *>(input_data), input_size))
        {
            fprintf(stderr, "Failed to load GLB from buffer: %s\n", err.c_str());
            return nullptr;
        }
        if (!warn.empty())
        {
            fprintf(stderr, "Warning: %s\n", warn.c_str());
        }

        // Perform selective compression
        auto result = selective_draco::SelectiveCompressGltf(model, *options);
        if (!result.ok())
        {
            fprintf(stderr, "Selective compression failed: %s\n", result.status().error_msg());
            return nullptr;
        }

        // Allocate output buffer and copy data
        auto &output_data = result.value();
        void *output = malloc(output_data.size());
        if (!output)
        {
            return nullptr;
        }

        memcpy(output, output_data.data(), output_data.size());
        *output_size = output_data.size();

        return output;
    }

    void *draco_decompress_gltf_to_buffer(const void *input_data, size_t input_size,
                                          size_t *output_size)
    {
        if (!input_data || !input_size || !output_size)
        {
            return nullptr;
        }

        *output_size = 0;

        // For decompression, we can use the existing Draco decoder since we produce valid Draco extensions
        // Load the glTF
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string err;
        std::string warn;

        if (!loader.LoadBinaryFromMemory(&model, &err, &warn, reinterpret_cast<const unsigned char *>(input_data), input_size))
        {
            fprintf(stderr, "Failed to load GLB from buffer: %s\n", err.c_str());
            return nullptr;
        }

        // Write uncompressed (the loader will automatically decompress Draco extensions)
        std::stringstream output_stream;
        if (!loader.WriteGltfSceneToStream(&model, output_stream, false, true))
        {
            fprintf(stderr, "Failed to write uncompressed glTF\n");
            return nullptr;
        }

        std::string output = output_stream.str();
        // Allocate output buffer
        void *result = malloc(output.size());
        if (!result)
        {
            return nullptr;
        }

        memcpy(result, output.data(), output.size());
        *output_size = output.size();

        return result;
    }

    void draco_free_buffer(void *buffer)
    {
        if (buffer)
        {
            free(buffer);
        }
    }

} // extern "C"
