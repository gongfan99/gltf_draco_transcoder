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
    lib_name = "draco_transcoder_shared"

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
_lib.draco_transcode_gltf.argtypes = [c_char_p, c_char_p, ctypes.POINTER(DracoOptions)]
_lib.draco_transcode_gltf.restype = c_int

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


def _has_unsupported_primitives(data: bytes) -> bool:
    """
    Check if the glB data contains any primitives with modes other than 0 (POINTS) or 4 (TRIANGLES).

    Args:
        data (bytes): The glB binary data

    Returns:
        bool: True if unsupported primitives are found
    """
    if len(data) < 12:
        return True  # Too short to be valid glB

    # Check magic
    magic = data[:4]
    if magic != b"glTF":
        return True  # Not a glB file

    try:
        # Parse glB header
        version = struct.unpack("<I", data[4:8])[0]
        total_length = struct.unpack("<I", data[8:12])[0]

        if version != 2:
            return True  # Not glTF 2.0

        # Skip header, read first chunk (JSON)
        offset = 12
        while offset + 8 <= len(data):
            chunk_length = struct.unpack("<I", data[offset + 4 : offset + 8])[0]

            # Try to decode as JSON regardless of chunk type (some glB files don't use 'JSON' exactly)
            try:
                json_data = data[
                    offset + 8 : offset + 8 + min(chunk_length, 50000)
                ]  # Reasonable limit
                json_str = json_data.decode("utf-8", errors="ignore")
                # Look for the meshes array
                if '"meshes"' in json_str:
                    # Find the start of JSON
                    json_start = json_str.find("{")
                    if json_start >= 0:
                        json_content = json_str[json_start:]
                        # Try to find a reasonable end
                        brace_count = 0
                        end_pos = 0
                        for i, char in enumerate(json_content):
                            if char == "{":
                                brace_count += 1
                            elif char == "}":
                                brace_count -= 1
                                if brace_count == 0:
                                    end_pos = i + 1
                                    break
                        if end_pos > 0:
                            try:
                                gltf_json = json.loads(json_content[:end_pos])

                                # Check all meshes and primitives
                                for mesh in gltf_json.get("meshes", []):
                                    for primitive in mesh.get("primitives", []):
                                        mode = primitive.get(
                                            "mode", 4
                                        )  # Default is 4 (TRIANGLES)
                                        if mode not in [0, 4]:  # 0=POINTS, 4=TRIANGLES
                                            return True
                                return False  # No unsupported primitives found
                            except json.JSONDecodeError:
                                pass  # Continue to next chunk
            except UnicodeDecodeError:
                pass  # Continue to next chunk

            offset += 8 + chunk_length

        return True  # No valid JSON chunk found

    except (struct.error, json.JSONDecodeError, UnicodeDecodeError):
        return True  # Invalid glB or JSON - treat as unsupported


def compress_gltf(
    input_data: str | io.BytesIO,
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
    if isinstance(input_data, str):
        if not os.path.exists(input_data):
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

    # Check for unsupported primitives (skip compression if found)
    if _has_unsupported_primitives(input_bytes):
        print(
            "Warning: Input contains unsupported primitive types (only TRIANGLES and POINTS are supported). Returning original data unchanged."
        )
        # Return a copy of the input data
        input_buffer.seek(0)
        return io.BytesIO(input_buffer.read())

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


def decompress_gltf(input_data: str | io.BytesIO) -> io.BytesIO:
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
    if isinstance(input_data, str):
        if not os.path.exists(input_data):
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


if __name__ == "__main__":
    # Simple test/example
    import sys

    if len(sys.argv) != 3:
        print("Usage: python draco_transcoder.py <input.gltf> <output.gltf>")
        print("Example: python draco_transcoder.py input.gltf output_compressed.gltf")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    try:
        # Compress the glTF file
        compressed_data = compress_gltf(input_file)

        # Save the compressed data to file
        with open(output_file, "wb") as f:
            f.write(compressed_data.getvalue())

        print(f"Successfully compressed {input_file} to {output_file}")

        # Example of decompression (round-trip test)
        decompressed_data = decompress_gltf(output_file)
        print(
            f"Successfully decompressed back to {len(decompressed_data.getvalue())} bytes"
        )

    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
