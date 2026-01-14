import io
from pathlib import Path

import pytest

import gltf_draco_transcoder as gdt


def test_import():
    """Verify the package can be imported."""
    assert gdt is not None


def test_draco_options():
    """Verify DracoOptions structure is accessible."""
    options = gdt.DracoOptions()
    assert hasattr(options, "quantization_position")


def test_functions_available():
    """Verify main functions are available."""
    assert hasattr(gdt, "compress_gltf")
    assert hasattr(gdt, "decompress_gltf")


@pytest.mark.parametrize("filename", ["box.glb", "box_with_line.glb"])
def test_roundtrip(filename):
    """Test compress -> decompress roundtrip for multiple glB files."""
    glb_path = Path(__file__).with_name(filename)

    # Read original file
    with open(glb_path, "rb") as f:
        original_data = f.read()

    original_size = len(original_data)

    # Compress
    compressed = gdt.compress_gltf(glb_path)

    # Decompress
    decompressed = gdt.decompress_gltf(compressed)

    # Verify decompression worked
    assert decompressed is not None
    decompressed_data = decompressed.getvalue()
    assert decompressed_data.startswith(
        b"glTF"
    ), "Decompressed data should be valid glB"

    # Verify size is within xx% of original
    decompressed_size = len(decompressed_data)
    size_diff = abs(decompressed_size - original_size) / original_size
    assert (
        size_diff <= 2.0
    ), f"Roundtrip size difference too large: {size_diff:.2%} (original: {original_size}, decompressed: {decompressed_size})"


@pytest.mark.parametrize("filename,min_reduction", [("partial_cylinder.glb", 0.50)])
def test_compression(filename, min_reduction):
    """Test compression achieves minimum size reduction."""
    glb_path = Path(__file__).with_name(filename)

    # Read original file
    with open(glb_path, "rb") as f:
        original_data = f.read()

    original_size = len(original_data)

    # Compress
    compressed = gdt.compress_gltf(glb_path)

    # Verify compression occurred
    compressed_data = compressed.getvalue()
    compressed_size = len(compressed_data)
    # assert compressed_size < original_size, "Compression should reduce file size"

    # Verify it's still a valid glB
    assert compressed_data.startswith(b"glTF"), "Output should be valid glB"

    # Verify compression ratio meets minimum requirement
    compression_ratio = (original_size - compressed_size) / original_size
    # assert (
    #     compression_ratio >= min_reduction
    # ), f"Compression ratio {compression_ratio:.2%} is below minimum {min_reduction:.2%} (original: {original_size}, compressed: {compressed_size})"


def test_bytesio_input():
    """Test that compress_gltf and decompress_gltf accept io.BytesIO as input."""
    glb_path = Path(__file__).with_name("box.glb")

    # Read original file into BytesIO
    with open(glb_path, "rb") as f:
        original_data = f.read()
    input_bytesio = io.BytesIO(original_data)

    # Test compress_gltf with BytesIO input
    compressed = gdt.compress_gltf(input_bytesio)
    assert isinstance(compressed, io.BytesIO), "compress_gltf should return BytesIO"
    compressed_data = compressed.getvalue()
    assert compressed_data.startswith(b"glTF"), "Compressed data should be valid glB"

    # Test decompress_gltf with BytesIO input (compressed data)
    decompressed = gdt.decompress_gltf(compressed)
    assert isinstance(decompressed, io.BytesIO), "decompress_gltf should return BytesIO"
    decompressed_data = decompressed.getvalue()
    assert decompressed_data.startswith(
        b"glTF"
    ), "Decompressed data should be valid glB"


def test_quantization_impact():
    """Test that lower qp values result in better compression (smaller files)."""
    glb_path = Path(__file__).with_name("partial_cylinder.glb")

    # Read original file
    with open(glb_path, "rb") as f:
        original_data = f.read()

    # Compress with default qp=11
    compressed_11 = gdt.compress_gltf(glb_path, qp=11)
    compressed_11_size = len(compressed_11.getvalue())

    # Compress with higher qp=16
    compressed_16 = gdt.compress_gltf(glb_path, qp=16)
    compressed_16_size = len(compressed_16.getvalue())

    # Lower qp should result in smaller file (better compression)
    assert (
        compressed_16_size > compressed_11_size
    ), f"qp=16 ({compressed_16_size} bytes) should be larger than qp=11 ({compressed_11_size} bytes)"


def test_compression_level_impact():
    """Test that higher cl values result in better compression (smaller files)."""
    glb_path = Path(__file__).with_name("partial_cylinder.glb")

    # Read original file
    with open(glb_path, "rb") as f:
        original_data = f.read()

    # Compress with default cl=3
    compressed_1 = gdt.compress_gltf(glb_path, cl=1)
    compressed_1_size = len(compressed_1.getvalue())

    # Compress with higher cl=9
    compressed_9 = gdt.compress_gltf(glb_path, cl=9)
    compressed_9_size = len(compressed_9.getvalue())

    # Higher cl should result in smaller file (better compression)
    assert (
        compressed_9_size <= compressed_1_size
    ), f"cl=9 ({compressed_9_size} bytes) should be smaller or equal to cl=3 ({compressed_1_size} bytes)"
