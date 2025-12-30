#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <iostream>
#include <vector>
#include <string>

using namespace emscripten;

#include "selective_draco_transcoder.h"

std::vector<char> arrayBufferToVector(const val &jsArrayBuffer)
{
    val jsUint8Array = val::global("Uint8Array").new_(jsArrayBuffer);
    return convertJSArrayToNumberVector<char>(jsUint8Array);
}

val vectorToArrayBuffer(const std::vector<char> &vec)
{
    val jsArrayBuffer = val::global("ArrayBuffer").new_(vec.size());
    val jsUint8Array = val::global("Uint8Array").new_(jsArrayBuffer);
    jsUint8Array.call<void>("set", val(typed_memory_view(vec.size(), vec.data())));
    return jsArrayBuffer;
}

// Wrapper functions for JavaScript interface
val compress_gltf(const val &input_val, int qp = 11, int qt = 10, int qn = 8, int qc = 8,
                  int qtg = 8, int qw = 8, int qg = 8, int cl = 7)
{
    // Convert input ArrayBuffer to vector
    std::vector<char> input_data = arrayBufferToVector(input_val);
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
    void *result = draco_transcode_gltf_from_buffer((const void *)input_data.data(), input_size, &options, &output_size);

    if (!result)
    {
        throw std::runtime_error("Draco transcoding failed");
    }

    // Create output vector from result
    std::vector<char> output_data((char *)result, (char *)result + output_size);

    // Free the C buffer
    draco_free_buffer(result);

    // Convert vector back to ArrayBuffer
    return vectorToArrayBuffer(output_data);
}

val decompress_gltf(const val &input_val)
{
    // Convert input ArrayBuffer to vector
    std::vector<char> input_data = arrayBufferToVector(input_val);
    size_t input_size = input_data.size();

    // Call the decompressor function
    size_t output_size;
    void *result = draco_decompress_gltf_to_buffer((const void *)input_data.data(), input_size, &output_size);

    if (!result)
    {
        throw std::runtime_error("Draco decompression failed");
    }

    // Create output vector from result
    std::vector<char> output_data((char *)result, (char *)result + output_size);

    // Free the C buffer
    draco_free_buffer(result);

    // Convert vector back to ArrayBuffer
    return vectorToArrayBuffer(output_data);
}

EMSCRIPTEN_BINDINGS(gltf_draco_transcoder)
{
    function("compress_gltf", &compress_gltf);
    function("decompress_gltf", &decompress_gltf);
}
