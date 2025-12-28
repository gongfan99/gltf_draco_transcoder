#!/bin/bash
set -e

cd draco_src
mkdir _gh_draco_sdk_build && cd _gh_draco_sdk_build
cmake .. -G "${CMAKE_GENERATOR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DDRACO_TRANSCODER_SUPPORTED=ON \
  -DDRACO_BUILD_EXECUTABLES=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_INSTALL_PREFIX=../../${DRACO_SDK_REL_PATH} \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build . ${CMAKE_BUILD_ARGS}
cd ..
cmake --install _gh_draco_sdk_build
cp -r third_party/eigen/Eigen ../${DRACO_SDK_REL_PATH}/include/
cp -r third_party/filesystem/include/ghc ../${DRACO_SDK_REL_PATH}/include/
cp third_party/tinygltf/*.h third_party/tinygltf/*.hpp ../${DRACO_SDK_REL_PATH}/include/ 2>/dev/null || true
