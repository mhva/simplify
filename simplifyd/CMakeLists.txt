set(SIMPLIFYD_SOURCES
    articleaction.cc
    contextaction.cc
    hash.cc
    httpquery.cc
    httpresponse.cc
    mongoose.c
    main.cc
    searchaction.cc
    server.cc)

set(REPOSITORY_SUFFIX ".config/simplify/repository" CACHE STRING "Directory of the default repository.")
set(WWW_ROOT "${CMAKE_INSTALL_PREFIX}/share/simplify/html" CACHE STRING "Directory that the web server will use as the root (www-root).")

configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_HOME_DIRECTORY})

if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++0x -Wall -fstrict-aliasing -Wstrict-aliasing=2")
endif (CMAKE_COMPILER_IS_GNUCXX)

add_executable(simplifyd ${SIMPLIFYD_SOURCES})
target_link_libraries(simplifyd simplify ${EXTRA_DEPS})
