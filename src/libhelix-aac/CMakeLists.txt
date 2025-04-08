cmake_minimum_required(VERSION 3.10)
project(libhelix-aac)

# Set the C standard
set(CMAKE_C_STANDARD 99)

# Do not use Arduino
add_definitions(-DUSE_DEFAULT_STDLIB)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/*.c"
    "${PROJECT_SOURCE_DIR}/*.h"
)

# Check if BUILD_SHARED_LIBS is enabled, build shared or static accordingly
if(BUILD_SHARED_LIBS)
    # Create the shared library
    add_library(helix-aac SHARED ${SOURCE_FILES})
else()
    # Create the static library
    add_library(helix-aac STATIC ${SOURCE_FILES})
endif()

# Set the output directory for libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Optionally, install the libraries
install(TARGETS helix-aac
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Specify the headers for installation
install(DIRECTORY "${PROJECT_SOURCE_DIR}"
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
