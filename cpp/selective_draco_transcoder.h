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

#ifndef SELECTIVE_DRACO_TRANSCODER_H_
#define SELECTIVE_DRACO_TRANSCODER_H_

#include <vector>
#include <unordered_map>
#include <set>
#include <stddef.h>

#include "tiny_gltf.h"

// Draco includes
#include "draco/compression/encode.h"
#include "draco/core/cycle_timer.h"
#include "draco/io/point_cloud_io.h"
#include "draco/mesh/mesh.h"

#ifdef _WIN32
#define DRACO_API __declspec(dllexport)
#else
#define DRACO_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    struct DracoOptions
    {
        int quantization_position;
        int quantization_tex_coord;
        int quantization_normal;
        int quantization_color;
        int quantization_tangent;
        int quantization_weight;
        int quantization_generic;
        int compression_level;
        int min_vertices;

#ifdef __cplusplus
        DracoOptions() : quantization_position(11),
                         quantization_tex_coord(10),
                         quantization_normal(8),
                         quantization_color(8),
                         quantization_tangent(8),
                         quantization_weight(8),
                         quantization_generic(8),
                         compression_level(7),
                         min_vertices(30) {}
#endif
    };

    // Function to transcode glTF from buffer with selective compression
    DRACO_API void *draco_transcode_gltf_from_buffer(const void *input_data,
                                                     size_t input_size,
                                                     const DracoOptions *options,
                                                     size_t *output_size);

    // Function to decompress Draco-compressed glTF
    DRACO_API void *draco_decompress_gltf_to_buffer(const void *input_data,
                                                    size_t input_size,
                                                    size_t *output_size);

    // Free buffer allocated by the above functions
    DRACO_API void draco_free_buffer(void *buffer);

#ifdef __cplusplus
}
#endif

#endif // SELECTIVE_DRACO_TRANSCODER_H_
