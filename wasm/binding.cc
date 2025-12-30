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

// Wrapper functions for JavaScript interface with options object
struct DracoOptionsJS
{
    int quantization_position = 11;
    int quantization_tex_coord = 10;
    int quantization_normal = 8;
    int quantization_color = 8;
    int quantization_tangent = 8;
    int quantization_weight = 8;
    int quantization_generic = 8;
    int compression_level = 7;
};

val compress_gltf(const val &input_val, const DracoOptionsJS &options)
{
    // Convert input ArrayBuffer to vector
    std::vector<char> input_data = arrayBufferToVector(input_val);
    size_t input_size = input_data.size();

    // Set up DracoOptions
    DracoOptions c_options;
    c_options.quantization_position = options.quantization_position;
    c_options.quantization_tex_coord = options.quantization_tex_coord;
    c_options.quantization_normal = options.quantization_normal;
    c_options.quantization_color = options.quantization_color;
    c_options.quantization_tangent = options.quantization_tangent;
    c_options.quantization_weight = options.quantization_weight;
    c_options.quantization_generic = options.quantization_generic;
    c_options.compression_level = options.compression_level;

    // Call the transcoder function
    size_t output_size;
    void *result = draco_transcode_gltf_from_buffer((const void *)input_data.data(), input_size, &c_options, &output_size);

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
    value_object<DracoOptionsJS>("DracoOptions")
        .field("quantization_position", &DracoOptionsJS::quantization_position)
        .field("quantization_tex_coord", &DracoOptionsJS::quantization_tex_coord)
        .field("quantization_normal", &DracoOptionsJS::quantization_normal)
        .field("quantization_color", &DracoOptionsJS::quantization_color)
        .field("quantization_tangent", &DracoOptionsJS::quantization_tangent)
        .field("quantization_weight", &DracoOptionsJS::quantization_weight)
        .field("quantization_generic", &DracoOptionsJS::quantization_generic)
        .field("compression_level", &DracoOptionsJS::compression_level);

    function("compress_gltf", &compress_gltf);
    function("compress_gltf", select_overload<val(const val &)>(
                                  [](const val &input_val)
                                  {
                                      return compress_gltf(input_val, DracoOptionsJS());
                                  }));
    function("decompress_gltf", &decompress_gltf);
}
