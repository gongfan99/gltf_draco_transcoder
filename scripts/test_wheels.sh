#!/bin/sh
set -e

python -m pip install --upgrade pip
pip install pytest
pip install --no-index --find-links dist gltf_draco_transcoder
pytest tests/
