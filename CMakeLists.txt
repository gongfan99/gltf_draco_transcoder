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
  add_library(draco_transcoder_shared STATIC
      cpp/selective_draco_transcoder.cc
  )
else()
  add_library(draco_transcoder_shared SHARED
      cpp/selective_draco_transcoder.cc
  )
endif()

target_link_libraries(draco_transcoder_shared PRIVATE draco::draco)

# add_executable(draco cpp/draco_selective_prototype.cc)

# target_link_libraries(draco PRIVATE draco::draco)

target_include_directories(draco_transcoder_shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
)

set_target_properties(draco_transcoder_shared PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# Install the library into the package directory (handled by scikit-build-core)
install(TARGETS draco_transcoder_shared
    LIBRARY DESTINATION .
    RUNTIME DESTINATION .
    ARCHIVE DESTINATION .
)
