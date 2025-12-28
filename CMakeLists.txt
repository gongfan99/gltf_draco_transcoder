cmake_minimum_required(VERSION 3.15)

project(gltf_draco_transcoder VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard to match Draco
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED DRACO_SDK_PATH AND DEFINED ENV{DRACO_SDK_PATH})
  set(DRACO_SDK_PATH "$ENV{DRACO_SDK_PATH}")
endif()

if(DEFINED DRACO_SDK_PATH AND EXISTS "${DRACO_SDK_PATH}")
  message(STATUS "Using Draco SDK from: ${DRACO_SDK_PATH}")

  set(draco_DIR "${DRACO_SDK_PATH}/share/cmake/draco")
  find_package(draco REQUIRED)

  add_library(draco_transcoder_shared SHARED
      cpp/draco_transcoder_lib.cc
      cpp/draco_transcoder_c_api.cc
  )

  target_link_libraries(draco_transcoder_shared PRIVATE draco::draco)

else()
  message(FATAL_ERROR "DRACO_SDK_PATH is not defined or the path does not exist. Please provide a valid Draco SDK path.")
endif()

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
