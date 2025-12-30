from .loader import DracoOptions, compress_gltf, decompress_gltf

try:
    from ._version import version as __version__
except ImportError:
    __version__ = "unknown"
