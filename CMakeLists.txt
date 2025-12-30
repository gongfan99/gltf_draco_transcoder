cmake_minimum_required(VERSION 3.15)

project(gltf_draco_transcoder VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard to match Draco
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find Draco via vcpkg or system packages first
find_package(draco CONFIG QUIET)

if(draco_FOUND)
  message(STATUS "Using Draco from vcpkg or system: ${draco_DIR}")
else()
  # Fallback to SDK path if not found
  if(NOT DEFINED DRACO_SDK_PATH AND DEFINED ENV{DRACO_SDK_PATH})
    set(DRACO_SDK_PATH "$ENV{DRACO_SDK_PATH}")
  endif()

  if(DEFINED DRACO_SDK_PATH AND EXISTS "${DRACO_SDK_PATH}")
    message(STATUS "Using Draco SDK from: ${DRACO_SDK_PATH}")
    set(draco_DIR "${DRACO_SDK_PATH}/share/cmake/draco")
    find_package(draco REQUIRED)
  else()
    message(FATAL_ERROR "Draco not found via vcpkg or system, and DRACO_SDK_PATH is not defined or does not exist. Please provide a valid Draco SDK path or ensure vcpkg is configured correctly.")
  endif()
endif()

if (EMSCRIPTEN)
  find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
  add_executable(${PROJECT_NAME}
      cpp/selective_draco_transcoder.cc
      wasm/binding.cc
  )
  target_include_directories(${PROJECT_NAME} PRIVATE ${TINYGLTF_INCLUDE_DIRS})
else()
  add_library(${PROJECT_NAME} SHARED
      cpp/selective_draco_transcoder.cc
  )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE draco::draco)

# add_executable(draco cpp/draco_selective_prototype.cc)

# target_link_libraries(draco PRIVATE draco::draco)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
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
        -sSTACK_SIZE=2mb
        -sINITIAL_MEMORY=32mb
        -sALLOW_MEMORY_GROWTH=1
        -sMAXIMUM_MEMORY=500mb
        -sENVIRONMENT=web,node
        --bind
        --emit-tsd "${TARGET}.d.ts"
    )
else()
  set_target_properties(${PROJECT_NAME} PROPERTIES
      VERSION ${PROJECT_VERSION}
      SOVERSION ${PROJECT_VERSION_MAJOR}
      POSITION_INDEPENDENT_CODE ON
  )
endif()


if (EMSCRIPTEN)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.js DESTINATION .)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.wasm DESTINATION .)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.d.ts DESTINATION .)
else()
  # Install the library into the package directory (handled by scikit-build-core)
  install(TARGETS ${PROJECT_NAME}
      LIBRARY DESTINATION .
      RUNTIME DESTINATION .
      ARCHIVE DESTINATION .
  )
endif ()
