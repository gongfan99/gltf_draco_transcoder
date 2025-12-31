set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)

set(VCPKG_ENV_PASSTHROUGH EMSDK)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")

set(VCPKG_CXX_FLAGS "-flto=thin")
set(VCPKG_C_FLAGS "-flto=thin")
set(VCPKG_LINKER_FLAGS "-flto=thin")
set(VCPKG_POLICY_EMPTY_PACKAGE enabled)

set(VCPKG_BUILD_TYPE release)