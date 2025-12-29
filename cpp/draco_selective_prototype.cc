// Prototype for selective Draco compression using tinygltf and Draco.
// This code demonstrates the complexity of manual glTF to Draco bridging.

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <set>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NOEXCEPT
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

// Draco includes
#include "draco/compression/encode.h"
#include "draco/core/cycle_timer.h"
#include "draco/io/point_cloud_io.h"
#include "draco/mesh/mesh.h"

namespace std
{
    template <>
    struct hash<std::pair<size_t, size_t>>
    {
        std::size_t operator()(const std::pair<size_t, size_t> &p) const
        {
            std::size_t h1 = std::hash<size_t>{}(p.first);
            std::size_t h2 = std::hash<size_t>{}(p.second);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };
}

namespace
{

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
                // Add more types as needed
            }
        }
        return data;
    }

    // Encode a primitive as Draco (for TRIANGLES/POINTS only)
    std::vector<uint8_t> EncodePrimitiveToDraco(const tinygltf::Model &model,
                                                const tinygltf::Primitive &primitive, int cl,
                                                int qp, int qn)
    {
        std::cout << "    Creating Draco mesh..." << std::endl;
        draco::Mesh mesh;

        // Get vertex count from POSITION attribute first (needed for index validation)
        if (primitive.attributes.find("POSITION") == primitive.attributes.end())
        {
            std::cerr << "    No POSITION attribute found" << std::endl;
            return {};
        }
        size_t vertex_count = model.accessors[primitive.attributes.at("POSITION")].count;
        mesh.set_num_points(vertex_count);

        // Handle indices
        if (primitive.indices >= 0)
        {
            const auto &indices_accessor = model.accessors[primitive.indices];
            std::cout << "    Processing indices, accessor type: " << indices_accessor.componentType << std::endl;
            if (indices_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                auto indices = GetAccessorData<uint32_t>(model, primitive.indices);
                std::cout << "    Got " << indices.size() << " UNSIGNED_INT indices" << std::endl;
                uint32_t max_idx = 0;
                for (auto idx : indices)
                {
                    if (idx > max_idx)
                        max_idx = idx;
                }
                std::cout << "    Max index value: " << max_idx << std::endl;
                draco::Mesh::Face face;
                for (size_t i = 0; i < indices.size(); i += 3)
                {
                    face[0] = indices[i];
                    face[1] = indices[i + 1];
                    face[2] = indices[i + 2];
                    mesh.AddFace(face);
                }
                std::cout << "    Added " << indices.size() / 3 << " faces" << std::endl;
            }
            else if (indices_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                auto indices = GetAccessorData<uint16_t>(model, primitive.indices);
                std::cout << "    Got " << indices.size() << " UNSIGNED_SHORT indices" << std::endl;
                uint32_t max_idx = 0;
                for (auto idx : indices)
                {
                    if (static_cast<uint32_t>(idx) > max_idx)
                        max_idx = static_cast<uint32_t>(idx);
                }
                std::cout << "    Max index value: " << max_idx << " (vertex count: " << vertex_count << ")" << std::endl;

                // Validate index range to prevent out-of-bounds access
                if (max_idx >= vertex_count)
                {
                    std::cerr << "    ERROR: Index " << max_idx << " is out of range for " << vertex_count << " vertices" << std::endl;
                    return {};
                }

                draco::Mesh::Face face;
                for (size_t i = 0; i < indices.size(); i += 3)
                {
                    face[0] = static_cast<uint32_t>(indices[i]);
                    face[1] = static_cast<uint32_t>(indices[i + 1]);
                    face[2] = static_cast<uint32_t>(indices[i + 2]);
                    mesh.AddFace(face);
                }
                std::cout << "    Added " << indices.size() / 3 << " faces" << std::endl;
            }
            else if (indices_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                auto indices = GetAccessorData<uint8_t>(model, primitive.indices);
                std::cout << "    Got " << indices.size() << " UNSIGNED_BYTE indices" << std::endl;
                uint32_t max_idx = 0;
                for (auto idx : indices)
                {
                    if (static_cast<uint32_t>(idx) > max_idx)
                        max_idx = static_cast<uint32_t>(idx);
                }
                std::cout << "    Max index value: " << max_idx << std::endl;
                draco::Mesh::Face face;
                for (size_t i = 0; i < indices.size(); i += 3)
                {
                    face[0] = static_cast<uint32_t>(indices[i]);
                    face[1] = static_cast<uint32_t>(indices[i + 1]);
                    face[2] = static_cast<uint32_t>(indices[i + 2]);
                    mesh.AddFace(face);
                }
                std::cout << "    Added " << indices.size() / 3 << " faces" << std::endl;
            }
            else
            {
                std::cerr << "    Unsupported index type: " << indices_accessor.componentType << std::endl;
                return {};
            }
        }
        else
        {
            std::cout << "    No indices, will generate sequentially" << std::endl;
        }

        // Add attributes
        if (primitive.attributes.find("POSITION") != primitive.attributes.end())
        {
            std::cout << "    Adding POSITION attribute..." << std::endl;
            auto positions = GetAccessorData<float>(model, primitive.attributes.at("POSITION"));
            std::cout << "    Got " << positions.size() << " position floats (" << positions.size() / 3 << " vertices)" << std::endl;
            draco::GeometryAttribute pos_attr;
            pos_attr.Init(draco::GeometryAttribute::POSITION, nullptr, 3, draco::DT_FLOAT32,
                          false, 3 * sizeof(float), 0);
            int pos_att_id = mesh.AddAttribute(pos_attr, true, positions.size() / 3);
            std::cout << "    Adding position values..." << std::endl;
            for (size_t i = 0; i < positions.size() / 3; ++i)
            {
                mesh.attribute(pos_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &positions[i * 3]);
            }
            std::cout << "    POSITION attribute added" << std::endl;
        }

        if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
        {
            std::cout << "    Adding NORMAL attribute..." << std::endl;
            auto normals = GetAccessorData<float>(model, primitive.attributes.at("NORMAL"));
            std::cout << "    Got " << normals.size() << " normal floats (" << normals.size() / 3 << " vertices)" << std::endl;
            draco::GeometryAttribute normal_attr;
            normal_attr.Init(draco::GeometryAttribute::NORMAL, nullptr, 3, draco::DT_FLOAT32,
                             false, 3 * sizeof(float), 0);
            int normal_att_id = mesh.AddAttribute(normal_attr, true, normals.size() / 3);
            std::cout << "    Adding normal values..." << std::endl;
            for (size_t i = 0; i < normals.size() / 3; ++i)
            {
                mesh.attribute(normal_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &normals[i * 3]);
            }
            std::cout << "    NORMAL attribute added" << std::endl;
        }

        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
        {
            std::cout << "    Adding TEXCOORD_0 attribute..." << std::endl;
            auto texcoords = GetAccessorData<float>(model, primitive.attributes.at("TEXCOORD_0"));
            std::cout << "    Got " << texcoords.size() << " texcoord floats (" << texcoords.size() / 2 << " vertices)" << std::endl;
            draco::GeometryAttribute texcoord_attr;
            texcoord_attr.Init(draco::GeometryAttribute::TEX_COORD, nullptr, 2, draco::DT_FLOAT32,
                               false, 2 * sizeof(float), 0);
            int texcoord_att_id = mesh.AddAttribute(texcoord_attr, true, texcoords.size() / 2);
            std::cout << "    Adding texcoord values..." << std::endl;
            for (size_t i = 0; i < texcoords.size() / 2; ++i)
            {
                mesh.attribute(texcoord_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &texcoords[i * 2]);
            }
            std::cout << "    TEXCOORD_0 attribute added" << std::endl;
        }

        if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
        {
            std::cout << "    Adding COLOR_0 attribute..." << std::endl;
            const auto &color_accessor = model.accessors[primitive.attributes.at("COLOR_0")];
            int component_count = tinygltf::GetNumComponentsInType(color_accessor.type);
            std::cout << "    Color has " << component_count << " components, type: " << color_accessor.componentType << std::endl;

            if (color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                auto colors = GetAccessorData<float>(model, primitive.attributes.at("COLOR_0"));
                std::cout << "    Got " << colors.size() << " color floats (" << colors.size() / component_count << " vertices)" << std::endl;
                draco::GeometryAttribute color_attr;
                color_attr.Init(draco::GeometryAttribute::COLOR, nullptr, component_count, draco::DT_FLOAT32,
                                false, component_count * sizeof(float), 0);
                int color_att_id = mesh.AddAttribute(color_attr, true, colors.size() / component_count);
                std::cout << "    Adding color values..." << std::endl;
                for (size_t i = 0; i < colors.size() / component_count; ++i)
                {
                    mesh.attribute(color_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &colors[i * component_count]);
                }
                std::cout << "    COLOR_0 attribute added" << std::endl;
            }
            else if (color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                auto colors = GetAccessorData<uint8_t>(model, primitive.attributes.at("COLOR_0"));
                std::cout << "    Got " << colors.size() << " color bytes (" << colors.size() / component_count << " vertices)" << std::endl;
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
                std::cout << "    Adding color values..." << std::endl;
                for (size_t i = 0; i < float_colors.size() / component_count; ++i)
                {
                    mesh.attribute(color_att_id)->SetAttributeValue(draco::AttributeValueIndex(i), &float_colors[i * component_count]);
                }
                std::cout << "    COLOR_0 attribute added" << std::endl;
            }
        }

        std::cout << "    Mesh created with " << mesh.num_faces() << " faces and " << mesh.num_points() << " points" << std::endl;

        // Encode
        std::cout << "    Setting up Draco encoder..." << std::endl;
        draco::Encoder encoder;
        encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, qp);
        encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, qn);
        encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, 10); // UV quantization
        encoder.SetAttributeQuantization(draco::GeometryAttribute::COLOR, 8);      // Color quantization
        int speed = 10 - cl;                                                       // Map compression level to speed (0=fast, 10=slow/best compression)
        encoder.SetSpeedOptions(speed, speed);
        std::cout << "    Encoder configured, speed=" << speed << std::endl;

        std::cout << "    Starting Draco encoding..." << std::endl;
        draco::EncoderBuffer buffer;
        const draco::Status status = encoder.EncodeMeshToBuffer(mesh, &buffer);
        if (!status.ok())
        {
            std::cerr << "Draco encoding failed: " << status.error_msg() << std::endl;
            return {};
        }

        std::cout << "    Draco encoding completed successfully" << std::endl;
        return {buffer.data(), buffer.data() + buffer.size()};
    }

} // namespace

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        std::cerr << "Usage: " << argv[0] << " <input.glb> <output.glb> <qp> <cl>" << std::endl;
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];
    int qp = std::stoi(argv[3]);
    int cl = std::stoi(argv[4]);
    int qn = 8; // Example normal quantization

    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    // Load GLB
    if (!loader.LoadBinaryFromFile(&model, &err, &warn, input_file))
    {
        std::cerr << "Failed to load GLB: " << err << std::endl;
        return 1;
    }
    if (!warn.empty())
    {
        std::cout << "Warning: " << warn << std::endl;
    }

    // First pass: Analyze accessor and bufferView usage to determine which primitives can be compressed
    std::cout << "Analyzing accessor and bufferView usage..." << std::endl;
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
            std::cout << "  Primitive " << prim_idx << " in mesh " << mesh_idx << " can_compress: " << (can_compress ? "YES" : "NO") << std::endl;
        }
    }

    // Third pass: Encode all compressible primitives FIRST (before any bufferView modifications)
    std::cout << "Processing meshes and primitives..." << std::endl;
    std::unordered_map<std::pair<size_t, size_t>, std::vector<uint8_t>> draco_bitstreams;

    for (size_t mesh_idx = 0; mesh_idx < model.meshes.size(); ++mesh_idx)
    {
        auto &mesh = model.meshes[mesh_idx];
        std::cout << "Processing mesh " << mesh_idx << " with " << mesh.primitives.size() << " primitives" << std::endl;

        for (size_t prim_idx = 0; prim_idx < mesh.primitives.size(); ++prim_idx)
        {
            auto &primitive = mesh.primitives[prim_idx];
            int mode = primitive.mode; // Default 4 = TRIANGLES

            std::cout << "  Primitive " << prim_idx << " has mode " << mode << std::endl;

            auto prim_key = std::make_pair(mesh_idx, prim_idx);
            if (can_compress_primitive[prim_key])
            {
                std::cout << "  Encoding primitive with Draco..." << std::endl;
                // Encode with Draco
                auto draco_data = EncodePrimitiveToDraco(model, primitive, cl, qp, qn);

                if (!draco_data.empty())
                {
                    std::cout << "  Draco encoding successful, size: " << draco_data.size() << " bytes" << std::endl;
                    // Store the Draco bitstream for later use in consolidation
                    draco_bitstreams[prim_key] = std::move(draco_data);

                    std::cout << "  Draco extension will be added in consolidation phase" << std::endl;
                }
                else
                {
                    std::cout << "  Draco encoding failed or empty" << std::endl;
                }
            }
            else
            {
                // Leave LINE_STRIP, LINE_LOOP, etc. unchanged
                std::cout << "  Skipping primitive (shared data or unsupported mode)" << std::endl;
            }
        }
    }

    // Fourth pass: Consolidate all data into a single buffer (AFTER encoding is complete)
    std::cout << "Consolidating all data into single buffer..." << std::endl;
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
    std::cout << "Processing " << model.bufferViews.size() << " buffer views..." << std::endl;
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
            std::cout << "  Skipping bufferView " << bv_idx << " (used by compressed primitive)" << std::endl;
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
                std::cout << "  Adding Draco extension for primitive " << prim_idx << " in mesh " << mesh_idx << std::endl;

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
                // Map attributes to Draco attribute IDs (POSITION=0, NORMAL=1, TEXCOORD_0=2, COLOR_0=3)
                int draco_attr_id = 0;
                if (primitive.attributes.find("POSITION") != primitive.attributes.end())
                {
                    attr_map["POSITION"] = tinygltf::Value(draco_attr_id++);
                }
                if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
                {
                    attr_map["NORMAL"] = tinygltf::Value(draco_attr_id++);
                }
                if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                {
                    attr_map["TEXCOORD_0"] = tinygltf::Value(draco_attr_id++);
                }
                if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
                {
                    attr_map["COLOR_0"] = tinygltf::Value(draco_attr_id++);
                }

                draco_ext["attributes"] = tinygltf::Value(attr_map);

                primitive.extensions["KHR_draco_mesh_compression"] = tinygltf::Value(draco_ext);

                std::cout << "  Draco extension added to primitive" << std::endl;
            }
        }
    }

    // Replace buffers with our single consolidated buffer
    model.buffers.clear();
    tinygltf::Buffer consolidated_gltf_buffer;
    consolidated_gltf_buffer.data = std::move(consolidated_buffer);
    model.buffers.push_back(std::move(consolidated_gltf_buffer));

    // Add extensions
    model.extensionsUsed.push_back("KHR_draco_mesh_compression");
    model.extensionsRequired.push_back("KHR_draco_mesh_compression");

    std::cout << "Writing output to " << output_file << std::endl;
    // Save GLB
    if (!loader.WriteGltfSceneToFile(&model, output_file, false, true, false, true))
    {
        std::cerr << "Failed to write GLB" << std::endl;
        return 1;
    }

    std::cout << "Successfully processed " << input_file << " -> " << output_file << std::endl;
    return 0;
}
