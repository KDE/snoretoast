cmake_minimum_required(VERSION 3.4)

project(snoretoast VERSION 0.7.0)
# Always change the guid when the version is changed SNORETOAST_CALLBACK_GUID
set(SNORETOAST_CALLBACK_GUID eb1fdd5b-8f70-4b5a-b230-998a2dc19303)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

option(BUILD_EXAMPLES "Whether to build the examples" OFF)
option(BUILD_STATIC_RUNTIME "Whether link statically to the msvc runtime" ON)

include(GenerateExportHeader)

include(SnoreMacros)
include(cmakerc/CMakeRC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

if (BUILD_STATIC_RUNTIME)
	#link runtime static
	if(MSVC)
		foreach(_bt DEBUG RELEASE RELWITHDEBINFO)
			string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_${_bt} ${CMAKE_CXX_FLAGS_${_bt}})
		endforeach(_bt DEBUG RELEASE RELWITHDEBINFO)
	endif(MSVC)
endif()

add_subdirectory(data)
add_subdirectory(src)

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
