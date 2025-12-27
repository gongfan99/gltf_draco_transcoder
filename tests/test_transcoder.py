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


# Add more functional tests if sample glTF data is available
