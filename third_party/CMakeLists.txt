set(SYSTEM_V8 FALSE CACHE BOOL "Use system v8 library")
set(SYSTEM_EB FALSE CACHE BOOL "Use system libeb")

add_custom_target(v8
    COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/v8
    COMMAND cd "${CMAKE_CURRENT_BINARY_DIR}/v8" && ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scons/scons.py -j 2 -Q -f ${CMAKE_CURRENT_SOURCE_DIR}/v8/SConstruct -Y ${CMAKE_CURRENT_SOURCE_DIR}/v8 debuggersupport=off profilingsupport=off)
add_custom_target(eb
    COMMAND cp -R ${CMAKE_CURRENT_SOURCE_DIR}/eb ${CMAKE_CURRENT_BINARY_DIR}/
    COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/eb && CFLAGS=${CMAKE_C_FLAGS_RELEASE} ./configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/ebroot --disable-ebnet --disable-nls --disable-samples --disable-pthread --enable-static --disable-shared && make -j 2 install)

if (NOT SYSTEM_V8)
    # Find python interpreter for running scons.
    find_package(PythonInterp REQUIRED)

    # On UNIX v8 requires us to link to pthreads.
    # TODO: Investigate if v8 requires something on Windows too.
    if (UNIX)
        find_package(Threads REQUIRED)
        set(EXTRA_DEPS "${CMAKE_THREAD_LIBS_INIT}" CACHE INTERNAL "")
    endif (UNIX)

    # FIXME: Unix only.
    set(V8_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/v8/include CACHE STRING "Path to v8 headers.")
    set(V8_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/v8/libv8.a CACHE STRING "Path to v8 library.")
else (NOT SYSTEM_V8)
    message(SEND_ERROR "Support for system v8 is not implemented yet.")
endif (NOT SYSTEM_V8)

if (NOT SYSTEM_EB)
    find_package(ZLIB REQUIRED)
    set(EXTRA_DEPS "${EXTRA_DEPS} ${ZLIB_LIBRARIES}" CACHE INTERNAL "")

    # FIXME: Unix only.
    set(EB_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/ebroot/include CACHE STRING "Path to eb library headers.")
    set(EB_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/ebroot/lib/libeb.a CACHE STRING "Path to eb library.")
else (NOT SYSTEM_EB)
    message(SEND_ERROR "Support for system libeb is not implemented yet.")
endif (NOT SYSTEM_EB)
