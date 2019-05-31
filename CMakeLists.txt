cmake_minimum_required(VERSION 3.4)

project( snoretoast VERSION 0.5.99)


option(BUILD_EXAMPLES "Whether to build the examples" OFF)

include(GenerateExportHeader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_subdirectory(data)
add_subdirectory(src)

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
