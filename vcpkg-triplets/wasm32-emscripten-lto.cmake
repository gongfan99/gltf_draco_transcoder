set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)

# 1. Tell vcpkg to "let through" the EMSDK variable from the shell
list(APPEND VCPKG_ENV_PASSTHROUGH "EMSDK")

# 2. Now $ENV{EMSDK} will actually have a value inside this script
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")

set(VCPKG_CXX_FLAGS "-flto")
set(VCPKG_C_FLAGS "-flto")