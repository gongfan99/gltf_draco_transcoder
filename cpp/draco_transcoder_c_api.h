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

#ifndef DRACO_TOOLS_DRACO_TRANSCODER_C_API_H_
#define DRACO_TOOLS_DRACO_TRANSCODER_C_API_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// C-compatible struct for Draco compression options.
// Mirrors the command-line options from draco_transcoder.cc
typedef struct {
  int quantization_position;   // -qp, default 11
  int quantization_tex_coord;  // -qt, default 10
  int quantization_normal;     // -qn, default 8
  int quantization_color;      // -qc, default 8
  int quantization_tangent;    // -qtg, default 8
  int quantization_weight;     // -qw, default 8
  int quantization_generic;    // -qg, default 8
  int compression_level;       // compression level, default 7
} DracoOptions;

// Transcodes a glTF file to Draco compressed glTF.
// Returns 0 on success, non-zero on error.
int draco_transcode_gltf(const char *input_filename,
                         const char *output_filename, DracoOptions *options);

// Transcodes glTF data from a memory buffer to Draco compressed glTF.
// Returns a pointer to the output buffer on success, NULL on error.
// The caller must free the returned buffer using draco_free_buffer().
void *draco_transcode_gltf_from_buffer(const void *input_data,
                                       size_t input_size, DracoOptions *options,
                                       size_t *output_size);

// Decompresses Draco compressed glTF data to uncompressed glTF.
// Returns a pointer to the output buffer on success, NULL on error.
// The caller must free the returned buffer using draco_free_buffer().
void *draco_decompress_gltf_to_buffer(const void *input_data, size_t input_size,
                                      size_t *output_size);

// Frees a buffer allocated by the C API functions.
void draco_free_buffer(void *buffer);

#ifdef __cplusplus
}
#endif

#endif  // DRACO_TOOLS_DRACO_TRANSCODER_C_API_H_
