cmake_minimum_required(VERSION 3.15)

project(gltf_draco_transcoder VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard to match Draco
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find Draco via vcpkg or system packages first
find_package(draco CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")

if (EMSCRIPTEN)
  add_executable(${PROJECT_NAME}
      cpp/selective_draco_transcoder.cc
      wasm/binding.cc
  )
else()
  add_library(${PROJECT_NAME} SHARED
      cpp/selective_draco_transcoder.cc
  )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE draco::draco)

target_include_directories(${PROJECT_NAME} 
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
  PRIVATE 
    ${TINYGLTF_INCLUDE_DIRS}
)

if (EMSCRIPTEN)
    target_compile_options (${PROJECT_NAME} PRIVATE
        $<IF:$<CONFIG:Release>,-sDISABLE_EXCEPTION_CATCHING=1,-sDISABLE_EXCEPTION_CATCHING=0>
    )
    target_link_options (${PROJECT_NAME} PRIVATE
        $<IF:$<CONFIG:Release>,-sDISABLE_EXCEPTION_CATCHING=1,-sDISABLE_EXCEPTION_CATCHING=0>
        # -g
        # -fsanitize=address
        # -sASSERTIONS=2
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sSTACK_SIZE=4mb
        -sINITIAL_MEMORY=64mb
        -sALLOW_MEMORY_GROWTH=1
        -sMAXIMUM_MEMORY=1000mb
        -sENVIRONMENT=web,node
        --bind
        --emit-tsd "${PROJECT_NAME}.d.ts"
    )
else()
  set_target_properties(${PROJECT_NAME} PROPERTIES
      VERSION ${PROJECT_VERSION}
      SOVERSION ${PROJECT_VERSION_MAJOR}
      POSITION_INDEPENDENT_CODE ON
  )
endif()


if (EMSCRIPTEN)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.js DESTINATION .)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.wasm DESTINATION .)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.d.ts DESTINATION .)
else()
  # Install the library into the package directory (handled by scikit-build-core)
  install(TARGETS ${PROJECT_NAME}
      LIBRARY DESTINATION .
      RUNTIME DESTINATION .
      ARCHIVE DESTINATION .
  )
endif ()
