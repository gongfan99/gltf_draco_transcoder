#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <iostream>
#include <vector>
#include <string>

using namespace emscripten;

#include "selective_draco_transcoder.h"

// Wrapper functions for JavaScript interface
val compress_gltf(val input_val, int qp = 11, int qt = 10, int qn = 8, int qc = 8,
                  int qtg = 8, int qw = 8, int qg = 8, int cl = 7)
{
    // Extract data from JavaScript Uint8Array
    std::vector<uint8_t> input_data = vecFromJSArray<uint8_t>(input_val);
    size_t input_size = input_data.size();

    // Set up DracoOptions
    DracoOptions options;
    options.quantization_position = qp;
    options.quantization_tex_coord = qt;
    options.quantization_normal = qn;
    options.quantization_color = qc;
    options.quantization_tangent = qtg;
    options.quantization_weight = qw;
    options.quantization_generic = qg;
    options.compression_level = cl;

    // Call the transcoder function
    size_t output_size;
    void *result = draco_transcode_gltf_from_buffer(input_data.data(), input_size, &options, &output_size);

    if (!result)
    {
        throw std::runtime_error("Draco transcoding failed");
    }

    // Create output Uint8Array and copy data
    val Uint8Array = val::global("Uint8Array");
    val output_val = Uint8Array.new_(output_size);
    val output_buffer = output_val["buffer"];
    uint8_t *output_ptr = (uint8_t *)output_buffer.as<uintptr_t>(0);

    // Copy the result to the JavaScript array
    std::memcpy(output_ptr, result, output_size);

    // Free the C buffer
    draco_free_buffer(result);

    return output_val;
}

val decompress_gltf(val input_val)
{
    // Extract data from JavaScript Uint8Array
    std::vector<uint8_t> input_data = vecFromJSArray<uint8_t>(input_val);
    size_t input_size = input_data.size();

    // Call the decompressor function
    size_t output_size;
    void *result = draco_decompress_gltf_to_buffer(input_data.data(), input_size, &output_size);

    if (!result)
    {
        throw std::runtime_error("Draco decompression failed");
    }

    // Create output Uint8Array and copy data
    val Uint8Array = val::global("Uint8Array");
    val output_val = Uint8Array.new_(output_size);
    val output_buffer = output_val["buffer"];
    uint8_t *output_ptr = (uint8_t *)output_buffer.as<uintptr_t>(0);

    // Copy the result to the JavaScript array
    std::memcpy(output_ptr, result, output_size);

    // Free the C buffer
    draco_free_buffer(result);

    return output_val;
}

EMSCRIPTEN_BINDINGS(gltf_draco_transcoder)
{
    function("compress_gltf", &compress_gltf);
    function("decompress_gltf", &decompress_gltf);
}
