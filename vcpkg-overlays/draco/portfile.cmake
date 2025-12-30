vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/google/draco.git
    REF 3abbc66fdf5597b1560c44ce7840aac76900b3f7
    PATCHES
        enable-dedup.patch
        fix-invalid-casting.patch
)

if(VCPKG_TARGET_IS_EMSCRIPTEN)
    set(ENV{EMSCRIPTEN} "${EMSCRIPTEN_ROOT}")
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

if(NOT VCPKG_TARGET_IS_EMSCRIPTEN)
    vcpkg_copy_tools(TOOL_NAMES draco_encoder draco_decoder AUTO_CLEAN)
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_copy_pdbs()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")