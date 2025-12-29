set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)

# Use a standard CMake variable that you will pass from the command line
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${EMSDK_PATH}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")

set(VCPKG_CXX_FLAGS "-flto")
set(VCPKG_C_FLAGS "-flto")
