project( snoretoast )
cmake_minimum_required(VERSION 2.8)


add_definitions(-DUNICODE -D_UNICODE )

#link runtime static
if(MSVC)
    foreach(_bt DEBUG RELEASE RELWITHDEBINFO)
        string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_${_bt} ${CMAKE_CXX_FLAGS_${_bt}})
    endforeach(_bt DEBUG RELEASE RELWITHDEBINFO)
endif(MSVC)

add_subdirectory(data)
add_subdirectory(src)
