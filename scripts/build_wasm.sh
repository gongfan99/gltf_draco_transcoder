#!/bin/sh

cmake --fresh -S . -B build_wasm \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=vcpkg-triplets/emscripten-wrapper.cmake \
  -DVCPKG_TARGET_TRIPLET=wasm32-emscripten-os \
  -DVCPKG_INSTALL_OPTIONS=--allow-unsupported \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
cmake --build build_wasm --config Release

