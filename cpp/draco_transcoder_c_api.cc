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

#include "draco/tools/draco_transcoder_c_api.h"

#include <cstdlib>

#include "draco/core/decoder_buffer.h"
#include "draco/core/encoder_buffer.h"
#include "draco/core/status.h"
#include "draco/io/gltf_decoder.h"
#include "draco/io/gltf_encoder.h"
#include "draco/scene/scene_utils.h"
#include "draco/tools/draco_transcoder_lib.h"

extern "C" {

int draco_transcode_gltf(const char *input_filename,
                         const char *output_filename, DracoOptions *options) {
  if (!input_filename || !output_filename || !options) {
    return -1;  // Invalid arguments
  }

  // Set up file options
  draco::DracoTranscoder::FileOptions file_options;
  file_options.input_filename = input_filename;
  file_options.output_filename = output_filename;

  // Set up transcoding options from C struct
  draco::DracoTranscodingOptions transcode_options;
  transcode_options.geometry.compression_level = options->compression_level;
  transcode_options.geometry.quantization_position.SetQuantizationBits(
      options->quantization_position);
  transcode_options.geometry.quantization_bits_tex_coord =
      options->quantization_tex_coord;
  transcode_options.geometry.quantization_bits_normal =
      options->quantization_normal;
  transcode_options.geometry.quantization_bits_color =
      options->quantization_color;
  transcode_options.geometry.quantization_bits_tangent =
      options->quantization_tangent;
  transcode_options.geometry.quantization_bits_weight =
      options->quantization_weight;
  transcode_options.geometry.quantization_bits_generic =
      options->quantization_generic;

  // Check options validity
  const draco::Status check_status = transcode_options.geometry.Check();
  if (!check_status.ok()) {
    return -2;  // Invalid options
  }

  // Create and run transcoder
  draco::StatusOr<std::unique_ptr<draco::DracoTranscoder>> dt_result =
      draco::DracoTranscoder::Create(transcode_options);
  if (!dt_result.ok()) {
    return -3;  // Failed to create transcoder
  }

  std::unique_ptr<draco::DracoTranscoder> dt = std::move(dt_result).value();

  const draco::Status transcode_status = dt->Transcode(file_options);
  if (!transcode_status.ok()) {
    return -4;  // Transcoding failed
  }

  return 0;  // Success
}

void *draco_transcode_gltf_from_buffer(const void *input_data,
                                       size_t input_size, DracoOptions *options,
                                       size_t *output_size) {
  if (!input_data || !input_size || !options || !output_size) {
    return nullptr;  // Invalid arguments
  }

  *output_size = 0;

  // Set up transcoding options from C struct
  draco::DracoTranscodingOptions transcode_options;
  transcode_options.geometry.compression_level = options->compression_level;
  transcode_options.geometry.quantization_position.SetQuantizationBits(
      options->quantization_position);
  transcode_options.geometry.quantization_bits_tex_coord =
      options->quantization_tex_coord;
  transcode_options.geometry.quantization_bits_normal =
      options->quantization_normal;
  transcode_options.geometry.quantization_bits_color =
      options->quantization_color;
  transcode_options.geometry.quantization_bits_tangent =
      options->quantization_tangent;
  transcode_options.geometry.quantization_bits_weight =
      options->quantization_weight;
  transcode_options.geometry.quantization_bits_generic =
      options->quantization_generic;

  // Check options validity
  const draco::Status check_status = transcode_options.geometry.Check();
  if (!check_status.ok()) {
    return nullptr;  // Invalid options
  }

  // Decode input glTF from buffer
  draco::DecoderBuffer input_buffer;
  input_buffer.Init(reinterpret_cast<const char *>(input_data), input_size);

  draco::GltfDecoder decoder;
  draco::StatusOr<std::unique_ptr<draco::Scene>> scene_result =
      decoder.DecodeFromBufferToScene(&input_buffer);
  if (!scene_result.ok()) {
    return nullptr;  // Decoding failed
  }
  std::unique_ptr<draco::Scene> scene = std::move(scene_result).value();

  // Apply compression settings to scene
  draco::SceneUtils::SetDracoCompressionOptions(&transcode_options.geometry,
                                                scene.get());

  // Encode to output buffer
  draco::EncoderBuffer output_buffer;
  draco::GltfEncoder encoder;
  draco::Status encode_status = encoder.EncodeToBuffer(*scene, &output_buffer);
  if (!encode_status.ok()) {
    return nullptr;  // Encoding failed
  }

  // Allocate output buffer and copy data
  void *result = malloc(output_buffer.size());
  if (!result) {
    return nullptr;  // Memory allocation failed
  }

  memcpy(result, output_buffer.data(), output_buffer.size());
  *output_size = output_buffer.size();

  return result;
}

void *draco_decompress_gltf_to_buffer(const void *input_data, size_t input_size,
                                      size_t *output_size) {
  if (!input_data || !input_size || !output_size) {
    return nullptr;  // Invalid arguments
  }

  *output_size = 0;

  // Decode input Draco glTF from buffer
  draco::DecoderBuffer input_buffer;
  input_buffer.Init(reinterpret_cast<const char *>(input_data), input_size);

  draco::GltfDecoder decoder;
  draco::StatusOr<std::unique_ptr<draco::Scene>> scene_result =
      decoder.DecodeFromBufferToScene(&input_buffer);
  if (!scene_result.ok()) {
    return nullptr;  // Decoding failed
  }
  std::unique_ptr<draco::Scene> scene = std::move(scene_result).value();

  // Encode to uncompressed output buffer
  draco::EncoderBuffer output_buffer;
  draco::GltfEncoder encoder;
  draco::Status encode_status = encoder.EncodeToBuffer(*scene, &output_buffer);
  if (!encode_status.ok()) {
    return nullptr;  // Encoding failed
  }

  // Allocate output buffer and copy data
  void *result = malloc(output_buffer.size());
  if (!result) {
    return nullptr;  // Memory allocation failed
  }

  memcpy(result, output_buffer.data(), output_buffer.size());
  *output_size = output_buffer.size();

  return result;
}

void draco_free_buffer(void *buffer) {
  if (buffer) {
    free(buffer);
  }
}

}  // extern "C"
