import os

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


def test_compress_box():
    """Test compression of a valid glB file."""
    test_dir = os.path.dirname(__file__)
    box_glb_path = os.path.join(test_dir, "box.glb")

    # Read original file
    with open(box_glb_path, "rb") as f:
        original_data = f.read()

    # Compress
    compressed = gdt.compress_gltf(box_glb_path)

    # Verify compression occurred (output should be smaller)
    compressed_data = compressed.getvalue()
    assert len(compressed_data) < len(
        original_data
    ), "Compression should reduce file size"

    # Verify it's still a valid glB (starts with magic)
    assert compressed_data.startswith(b"glTF"), "Output should be valid glB"


def test_fallback_box_with_line():
    """Test that files with unsupported primitives return original data unchanged."""
    test_dir = os.path.dirname(__file__)
    box_with_line_path = os.path.join(test_dir, "box_with_line.glb")

    # Read original file
    with open(box_with_line_path, "rb") as f:
        original_data = f.read()

    # Attempt compression (should return original)
    result = gdt.compress_gltf(box_with_line_path)

    # Verify returned data is identical to original
    result_data = result.getvalue()
    assert (
        result_data == original_data
    ), "Unsupported primitives should return original data unchanged"


def test_roundtrip_box():
    """Test compress -> decompress roundtrip with box.glb."""
    test_dir = os.path.dirname(__file__)
    box_glb_path = os.path.join(test_dir, "box.glb")

    # Compress
    compressed = gdt.compress_gltf(box_glb_path)

    # Decompress
    decompressed = gdt.decompress_gltf(compressed)

    # Verify decompression worked
    assert decompressed is not None
    decompressed_data = decompressed.getvalue()
    assert decompressed_data.startswith(
        b"glTF"
    ), "Decompressed data should be valid glB"

    # Verify it's larger than compressed (normal Draco behavior)
    compressed_data = compressed.getvalue()
    assert len(decompressed_data) > len(
        compressed_data
    ), "Decompressed should be larger than compressed"
