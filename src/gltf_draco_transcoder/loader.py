#!/usr/bin/env python3
"""
Python wrapper for Draco glTF transcoder using ctypes.

This module provides a simple interface to compress glTF files using Draco compression.
"""

import ctypes
import io
import json
import os
import platform
import struct
import sysconfig
from ctypes import Structure, c_char_p, c_int
from pathlib import Path


class DracoOptions(Structure):
    """C-compatible struct for Draco compression options."""

    _fields_ = [
        ("quantization_position", c_int),
        ("quantization_tex_coord", c_int),
        ("quantization_normal", c_int),
        ("quantization_color", c_int),
        ("quantization_tangent", c_int),
        ("quantization_weight", c_int),
        ("quantization_generic", c_int),
        ("compression_level", c_int),
    ]


def _load_library() -> ctypes.CDLL:
    """Load the Draco transcoder shared library."""
    system = platform.system().lower()
    lib_name = "gltf_draco_transcoder"

    if system == "windows":
        lib_name = f"{lib_name}.dll"
    elif system == "darwin":
        lib_name = f"lib{lib_name}.dylib"
    else:  # Linux and others
        lib_name = f"lib{lib_name}.so"

    # Try to load from the same directory as this file (installed package)
    this_dir = Path(__file__).parent
    candidates = [
        Path(__file__).with_name(lib_name),
        Path(sysconfig.get_paths()["purelib"]) / "gltf_draco_transcoder" / lib_name,
    ]

    for candidate in candidates:
        if candidate.exists():
            return ctypes.CDLL(str(candidate))

    raise RuntimeError(f"Could not find Draco transcoder library: {lib_name}")


# Load the library
_lib = _load_library()

# Configure function signatures
_lib.draco_transcode_gltf_from_buffer.argtypes = [
    ctypes.c_void_p,
    ctypes.c_size_t,
    ctypes.POINTER(DracoOptions),
    ctypes.POINTER(ctypes.c_size_t),
]
_lib.draco_transcode_gltf_from_buffer.restype = ctypes.c_void_p

_lib.draco_decompress_gltf_to_buffer.argtypes = [
    ctypes.c_void_p,
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_size_t),
]
_lib.draco_decompress_gltf_to_buffer.restype = ctypes.c_void_p

_lib.draco_free_buffer.argtypes = [ctypes.c_void_p]
_lib.draco_free_buffer.restype = None


def compress_gltf(
    input_data: str | Path | io.BytesIO,
    qp: int = 11,
    qt: int = 10,
    qn: int = 8,
    qc: int = 8,
    qtg: int = 8,
    qw: int = 8,
    qg: int = 8,
    cl: int = 7,
) -> io.BytesIO:
    """
    Compress glTF data using Draco compression.

    Args:
        input_data (str or io.BytesIO): Input glTF data - either a file path (str) or BytesIO object
        qp (int): Quantization bits for position attribute (default: 11)
        qt (int): Quantization bits for texture coordinate attribute (default: 10)
        qn (int): Quantization bits for normal vector attribute (default: 8)
        qc (int): Quantization bits for color attribute (default: 8)
        qtg (int): Quantization bits for tangent attribute (default: 8)
        qw (int): Quantization bits for weight attribute (default: 8)
        qg (int): Quantization bits for generic attribute (default: 8)
        cl (int): Compression level [0-10] (default: 7)

    Returns:
        io.BytesIO: Compressed glTF data

    Raises:
        RuntimeError: If input data is invalid or compression fails
    """
    # Handle input data
    if isinstance(input_data, str) or isinstance(input_data, Path):
        if not Path(input_data).exists():
            raise RuntimeError(f"Input file does not exist: {input_data}")
        # Read file into BytesIO
        with open(input_data, "rb") as f:
            input_buffer = io.BytesIO(f.read())
    elif isinstance(input_data, io.BytesIO):
        input_buffer = input_data
    else:
        raise RuntimeError("input_data must be a file path (str) or BytesIO object")

    # Create options struct
    options = DracoOptions()
    options.quantization_position = qp
    options.quantization_tex_coord = qt
    options.quantization_normal = qn
    options.quantization_color = qc
    options.quantization_tangent = qtg
    options.quantization_weight = qw
    options.quantization_generic = qg
    options.compression_level = cl

    # Get input data
    input_bytes = input_buffer.getvalue()
    input_size = len(input_bytes)

    # Call the C function
    output_size = ctypes.c_size_t()
    result = _lib.draco_transcode_gltf_from_buffer(
        input_bytes, input_size, ctypes.byref(options), ctypes.byref(output_size)
    )

    if not result:
        raise RuntimeError("Draco transcoding failed")

    try:
        # Copy the result to Python bytes and return BytesIO
        output_data = ctypes.string_at(result, output_size.value)
        return io.BytesIO(output_data)
    finally:
        # Always free the C buffer
        _lib.draco_free_buffer(result)


def decompress_gltf(input_data: str | Path | io.BytesIO) -> io.BytesIO:
    """
    Decompress Draco-compressed glTF data to uncompressed glTF.

    Args:
        input_data (str or io.BytesIO): Input compressed glTF data - either a file path (str) or BytesIO object

    Returns:
        io.BytesIO: Decompressed glTF data

    Raises:
        RuntimeError: If input data is invalid or decompression fails
    """
    # Handle input data
    if isinstance(input_data, str) or isinstance(input_data, Path):
        if not Path(input_data).exists():
            raise RuntimeError(f"Input file does not exist: {input_data}")
        # Read file into BytesIO
        with open(input_data, "rb") as f:
            input_buffer = io.BytesIO(f.read())
    elif isinstance(input_data, io.BytesIO):
        input_buffer = input_data
    else:
        raise RuntimeError("input_data must be a file path (str) or BytesIO object")

    # Get input data
    input_bytes = input_buffer.getvalue()
    input_size = len(input_bytes)

    # Call the C function
    output_size = ctypes.c_size_t()
    result = _lib.draco_decompress_gltf_to_buffer(
        input_bytes, input_size, ctypes.byref(output_size)
    )

    if not result:
        raise RuntimeError("Draco decompression failed")

    try:
        # Copy the result to Python bytes and return BytesIO
        output_data = ctypes.string_at(result, output_size.value)
        return io.BytesIO(output_data)
    finally:
        # Always free the C buffer
        _lib.draco_free_buffer(result)


def main():
    """Command-line interface for Draco transcoder."""
    import sys

    if len(sys.argv) != 3:
        print("Usage: python -m gltf_draco_transcoder <input.gltf> <output.gltf>")
        print(
            "Example: python -m gltf_draco_transcoder input.gltf output_compressed.gltf"
        )
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    # Get input file size
    input_size = os.path.getsize(input_file)
    input_size_mb = input_size / (1024 * 1024)
    print(f"Input file: {input_file} ({input_size_mb:.2f} MB)")

    try:
        # Compress the glTF file
        compressed_data = compress_gltf(input_file)

        # Save the compressed data to file
        with open(output_file, "wb") as f:
            f.write(compressed_data.getvalue())

        # Get output file size
        output_size = os.path.getsize(output_file)
        output_size_mb = output_size / (1024 * 1024)
        print(f"Output file: {output_file} ({output_size_mb:.2f} MB)")

        compression_ratio = (
            (1 - output_size / input_size) * 100 if input_size > 0 else 0
        )
        print(f"Compression ratio: {compression_ratio:.1f}%")

    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
