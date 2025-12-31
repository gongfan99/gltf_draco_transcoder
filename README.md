# gltf_draco_transcoder

A high-performance Python package for compressing and decompressing glTF/glb files using the Draco 3D data compression library. This package provides both a Python API and a command-line interface for efficient 3D model compression.

## Features

- **Compression**: Compress glb files using Draco compression to reduce file sizes significantly
- **Decompression**: Decompress Draco-compressed glb files back to standard glTF format
- **High Performance**: C++ backend with Python bindings for maximum performance
- **Cross-platform**: Supports Windows, macOS (ARM64), and Linux (including ARM64)
- **Configurable**: Fine-tune compression quality with quantization and compression level settings
- **Easy Integration**: Simple Python API and command-line interface

## Installation

Install from PyPI:

```bash
pip install gltf_draco_transcoder
```

## Usage

### Python API

#### Basic Compression

```python
from gltf_draco_transcoder import compress_gltf, decompress_gltf

# Compress a glTF/glb file
compressed_data = compress_gltf('input.glb')

# Save compressed data
with open('output_compressed.glb', 'wb') as f:
    f.write(compressed_data.getvalue())

# Decompress back to glTF/glb
decompressed_data = decompress_gltf('output_compressed.glb')
with open('output_decompressed.glb', 'wb') as f:
    f.write(decompressed_data.getvalue())
```

#### Advanced Compression with Custom Options

```python
from gltf_draco_transcoder import compress_gltf, DracoOptions

# Create custom compression options
options = DracoOptions()
options.quantization_position = 14  # Higher quality for positions
options.quantization_normal = 10    # Higher quality for normals
options.compression_level = 9       # Higher compression effort

# Compress with custom settings
compressed_data = compress_gltf('input.glb', qp=14, qn=10, cl=9)
```

### Command Line Interface

Compress a glTF/glb file:

```bash
python -m gltf_draco_transcoder input.glb output_compressed.glb
```

This will display compression statistics including file sizes and compression ratio.

### Compression Options

The `compress_gltf` function accepts the following parameters:

- **Quantization Bits** (default values shown):

  - `qp=11`: Position quantization
  - `qt=10`: Texture coordinate quantization
  - `qn=8`: Normal vector quantization
  - `qc=8`: Color quantization
  - `qtg=8`: Tangent quantization
  - `qw=8`: Weight quantization
  - `qg=8`: Generic attribute quantization

- **Compression Level**:
  - `cl=7`: Compression effort (0-10, higher = better compression but slower)

Higher quantization bits = better quality but larger file sizes.
Higher compression levels = better compression ratios but slower processing.

## Development

### Building from Source

This project uses `scikit-build-core` with CMake and vcpkg for dependency management.

#### Prerequisites

- Python 3.10+
- CMake 3.15+
- Git
- vcpkg (automatically installed on Linux via CI scripts)

#### Build Steps

```bash
# Clone the repository
git clone https://github.com/gongfan99/gltf_draco_transcoder.git
cd gltf_draco_transcoder

# Install build dependencies
pip install scikit-build-core setuptools-scm

# Build the package
python -m build
```

### Project Structure

```
src/gltf_draco_transcoder/
├── __init__.py          # Main API exports
├── loader.py            # Python wrapper and ctypes bindings
├── py.typed             # Type hints marker
└── _version.py          # Version information (auto-generated)

cpp/
├── draco_transcoder_c_api.*    # C API wrapper
└── draco_transcoder_lib.*      # Core C++ implementation

vcpkg-overlays/draco/           # Custom Draco build configuration
vcpkg-triplets/                 # Platform-specific build triplets
```

## Platform Support

- **Linux**: x86_64 and ARM64 (manylinux/musllinux wheels)
- **macOS**: ARM64
- **Windows**: x86_64 (win_amd64 wheels)

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues on GitHub.

## License

This project is licensed under the Apache License Version 2.0 - see the [LICENSE](LICENSE) file for details.

## Credits

- Built on the [Draco](https://github.com/google/draco) 3D data compression library
- Uses [tinygltf](https://github.com/syoyo/tinygltf) for glTF file I/O
- Uses [vcpkg](https://github.com/microsoft/vcpkg) for dependency management
- Packaging with [scikit-build-core](https://github.com/scikit-build/scikit-build-core) and [cibuildwheel](https://github.com/pypa/cibuildwheel)
