cmake_minimum_required(VERSION 3.0.0)

project(SnoreToastQtExample VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Network REQUIRED)

if (NOT TARGET SnoreToast::LibSnoreToast)
    find_package(LibSnoreToast REQUIRED)
endif()

add_executable(${PROJECT_NAME} "main.cpp")

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Gui SnoreToast::LibSnoreToast SnoreToast::LibSnoreToastActions)
