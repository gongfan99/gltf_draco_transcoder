/*******************************************
 * This file is for debugging and testing
 * Uncomment the part in CMakeLists.txt to
 * build this file as an executable
 ******************************************/
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cstring>

#include "selective_draco_transcoder.h"

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input_glb_file> <output_glb_file> [compression_level]" << std::endl;
        return 1;
    }

    // Read input GLB file
    std::ifstream infile(argv[1], std::ios::binary);
    if (!infile.is_open())
    {
        std::cerr << "Error: Cannot open input file " << argv[1] << std::endl;
        return 1;
    }

    std::vector<unsigned char> input_data((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
    infile.close();

    if (input_data.empty())
    {
        std::cerr << "Error: Input file is empty or failed to read" << std::endl;
        return 1;
    }

    DracoOptions draco_options{};
    draco_options.quantization_position = 13;
    draco_options.compression_level = 7;

    if (argc >= 4)
    {
        try
        {
            draco_options.compression_level = std::stoi(argv[3]);
        }
        catch (const std::exception &)
        {
            std::cerr << "Error: Invalid compression_level value: " << argv[3] << std::endl;
            return 1;
        }
    }

    // Transcode with Draco
    size_t output_size;
    void *output_buffer = draco_transcode_gltf_from_buffer(input_data.data(), input_data.size(), &draco_options, &output_size);

    if (!output_buffer)
    {
        std::cerr << "Error: Draco transcoding failed" << std::endl;
        return 1;
    }

    // Write to output file
    std::ofstream outfile(argv[2], std::ios::binary);
    if (!outfile.is_open())
    {
        std::cerr << "Error: Cannot open output file " << argv[2] << std::endl;
        draco_free_buffer(output_buffer);
        return 1;
    }

    outfile.write(static_cast<const char *>(output_buffer), output_size);
    if (!outfile)
    {
        std::cerr << "Error: Failed to write to output file " << argv[2] << std::endl;
        outfile.close();
        draco_free_buffer(output_buffer);
        return 1;
    }

    outfile.close();
    draco_free_buffer(output_buffer);

    std::cout << "Successfully transcoded GLB file to " << argv[2] << std::endl;

    return 0;
}
