cmake_minimum_required (VERSION 2.8)
project (simplify
         LANGUAGES C CXX)
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party")

add_subdirectory("${THIRD_PARTY_DIR}/libeb")
add_subdirectory(simplify)
add_subdirectory(simplifyd)

install(DIRECTORY scripts DESTINATION share/simplify)