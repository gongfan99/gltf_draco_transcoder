set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)
# Add LTO flags to all builds
set(VCPKG_CXX_FLAGS "-flto")
set(VCPKG_C_FLAGS "-flto")
