cmake_minimum_required(VERSION 3.15)

project(gltf_draco_transcoder VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard to match Draco
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check if we should use a pre-built Draco SDK
if(DEFINED DRACO_SDK_PATH AND EXISTS "${DRACO_SDK_PATH}")
  message(STATUS "Using pre-built Draco SDK from: ${DRACO_SDK_PATH}")

  # Find the SDK library
  if(MSVC)
    find_library(DRACO_SDK_LIBRARY NAMES draco.lib PATHS "${DRACO_SDK_PATH}/lib" NO_DEFAULT_PATH REQUIRED)
  else()
    find_library(DRACO_SDK_LIBRARY NAMES draco libdraco PATHS "${DRACO_SDK_PATH}/lib" NO_DEFAULT_PATH REQUIRED)
  endif()

  # Add SDK include directory
  include_directories("${DRACO_SDK_PATH}/include")

  # Create the shared library
  add_library(draco_transcoder_shared SHARED
      cpp/draco_transcoder_lib.cc
      cpp/draco_transcoder_c_api.cc
  )

  # Link against Draco SDK
  target_link_libraries(draco_transcoder_shared PRIVATE ${DRACO_SDK_LIBRARY})

else()
  message(FATAL_ERROR "DRACO_SDK_PATH is not defined or the path does not exist. Please provide a valid Draco SDK path.")
endif()

# Include directories for headers
target_include_directories(draco_transcoder_shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
)

# Set library properties
set_target_properties(draco_transcoder_shared PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    # Ensure PIC for shared libraries
    POSITION_INDEPENDENT_CODE ON
)

# Install the library into the package directory (handled by scikit-build-core)
install(TARGETS draco_transcoder_shared
    LIBRARY DESTINATION .
    RUNTIME DESTINATION .
    ARCHIVE DESTINATION .
)
