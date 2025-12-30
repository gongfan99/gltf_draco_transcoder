vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/google/draco.git
    REF 3abbc66fdf5597b1560c44ce7840aac76900b3f7
    PATCHES
        fix-various-things.patch
)

if(VCPKG_TARGET_IS_EMSCRIPTEN)
    set(ENV{EMSCRIPTEN} "$ENV{EMSDK}/upstream/emscripten")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DDRACO_WASM=ON
        -DPYTHON_EXECUTABLE=:
        -DDRACO_JS_GLUE=OFF
        -DDRACO_TRANSCODER_SUPPORTED=ON
        -DDRACO_BUILD_EXECUTABLES=OFF
        -DDRACO_EIGEN_PATH=${CURRENT_INSTALLED_DIR}/include/eigen3
        -DDRACO_FILESYSTEM_PATH=${CURRENT_INSTALLED_DIR}/include
        -DDRACO_TINYGLTF_PATH=${CURRENT_INSTALLED_DIR}/include
)

vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_copy_pdbs()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")