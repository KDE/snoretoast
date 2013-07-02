project( snoretoast )
cmake_minimum_required(VERSION 2.8)


add_definitions(-DUNICODE -D_UNICODE )


add_subdirectory(data)
add_subdirectory(src)
