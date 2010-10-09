include(CheckCXXSourceCompiles)

set(LIBSIMPLIFY_SOURCES
    #    c/dictionary.cc
    #    c/error.cc
    #    c/repository.cc
    config.cc
    error.cc
    dictionary.cc
    epwing-dictionary.cc
    repository.cc
    utils.cc)

if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++0x -Wall -fstrict-aliasing -Wstrict-aliasing=2")
endif (CMAKE_COMPILER_IS_GNUCXX)

check_cxx_source_compiles("
#include <fcntl.h>
int main() { posix_fadvise(0, 0, 0, POSIX_FADV_SEQUENTIAL); }
" HAVE_POSIX_FADVISE)

configure_file(config.h.in config.h)

include_directories(${V8_INCLUDE_DIR} ${EB_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_HOME_DIRECTORY})

add_library(simplify STATIC ${LIBSIMPLIFY_SOURCES})
target_link_libraries(simplify ${V8_LIBRARY} ${EB_LIBRARY})
