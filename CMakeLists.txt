# CMakeLists.txt for 5CB Liquid Crystal Detector with Electrometer (Linux Optimized)

cmake_minimum_required(VERSION 3.16...3.21)
project(LCDetector VERSION 1.0)

# Enforce C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure compiler options for Linux
if(CMAKE_COMPILER_IS_GNUCXX)
  # Common options
  add_compile_options(-Wall -Wextra -pedantic)
  
  # Apply optimizations for Release builds
  if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_options(-O3 -march=native -ffast-math)
    message(STATUS "Using optimized build with CPU-specific optimizations")
  else()
    message(STATUS "Using debug build - optimizations disabled")
  endif()
endif()

# Find Geant4
find_package(Geant4 REQUIRED ui_all vis_all)

# Include Geant4 configuration
include(${Geant4_USE_FILE})

# Option for multi-threading (enabled by default)
option(USE_MULTITHREADING "Enable Geant4 multi-threading" ON)
if(USE_MULTITHREADING)
  add_definitions(-DG4MULTITHREADED)
  message(STATUS "Multi-threading enabled - Optimized for multi-core CPU")
else()
  message(STATUS "Multi-threading disabled")
endif()

# Source files - use GLOB on Linux for convenience
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB HEADERS ${PROJECT_SOURCE_DIR}/include/*.hh)

# Create executable
add_executable(LCDetector ${SOURCES} ${HEADERS})
target_link_libraries(LCDetector ${Geant4_LIBRARIES})

# Include directories
target_include_directories(LCDetector PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Create macros directory in build dir if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/macros)

# Copy all macro files to build directory
file(GLOB MAC_FILES "${PROJECT_SOURCE_DIR}/macros/*.mac")
foreach(_macro ${MAC_FILES})
  get_filename_component(_macro_name ${_macro} NAME)
  configure_file(
    ${_macro}
    ${CMAKE_BINARY_DIR}/macros/${_macro_name}
    COPYONLY
  )
endforeach()

# Output information
message(STATUS "CMAKE_GENERATOR: ${CMAKE_GENERATOR}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "Geant4_VERSION: ${Geant4_VERSION}")
message(STATUS "Linux build - Optimized with explicit electrometer modeling")
