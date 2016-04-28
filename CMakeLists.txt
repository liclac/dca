cmake_minimum_required(VERSION 3.1)
project(dca VERSION 0.0.1)

# Put binaries in /bin, libraries in /lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Default to C11
set(CMAKE_C_STANDARD 11)

# Include headers from /include, build sources from /src
include_directories(include)
add_subdirectory(src)
