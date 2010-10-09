# TODO: Require CMake >= 2.8.3 once it's out. It includes fix for
# newer versions of GTK2+ which has separated gdk-pixbuf library.
cmake_minimum_required(VERSION 2.8)
project(Simplify C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_HOME_DIRECTORY}/cmake)

if (UNIX)
    add_definitions(-DPOSIX)
elseif (WIN32)
    add_definitions(-DWIN32)
endif (UNIX)

if (APPLE)
    add_definitions(-DAPPLE)
endif (APPLE)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DLINUX)
endif (CMAKE_SYSTEM_NAME STREQUAL "Linux")

add_subdirectory(third_party)
add_subdirectory(simplify)
add_subdirectory(simplifyd)
