find_package(Threads)

include_directories(
  "${THIRD_PARTY_DIR}/v8/include"
  "${THIRD_PARTY_DIR}/libeb"
  "${THIRD_PARTY_DIR}/nowide_standalone"
  "${THIRD_PARTY_DIR}/json"
  "${CMAKE_SOURCE_DIR}"
  )

set(LIBSIMPLIFY_SOURCES
  "epwing/epwing-dictionary.cc"
  "dictionary.cc"
  "error.cc"
  "repository.cc"
  "simplify.cc"
  "utils.cc"
  )

add_library(simplify STATIC ${LIBSIMPLIFY_SOURCES})

set_property(TARGET simplify PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET simplify PROPERTY CXX_STANDARD 17)

if (NOT MSVC)
  # Disable RTTI because v8 is compiled without it.
  target_compile_options(simplify PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
endif ()

if (UNIX)
  target_compile_definitions(simplify PUBLIC -DSIMPLIFY_POSIX)
endif ()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  target_compile_definitions(simplify PUBLIC -DSIMPLIFY_LINUX)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  target_compile_definitions(simplify PUBLIC -DSIMPLIFY_DARWIN)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_compile_definitions(simplify PUBLIC -DSIMPLIFY_WINDOWS)
endif ()

target_link_libraries(simplify
  PUBLIC ${CMAKE_THREAD_LIBS_INIT}
  PUBLIC ${CMAKE_DL_LIBS}
  PUBLIC eb
  PUBLIC "${THIRD_PARTY_DIR}/v8/out.gn/x64.release/obj/libv8_monolith.a"
  )