include_directories("${CMAKE_SOURCE_DIR}")
add_definitions(
  -DSIMPLIFY_WWWROOT="${CMAKE_INSTALL_PREFIX}/share/simplify/html"
  )

set(SIMPLIFYD_SOURCES
  "articleaction.cc"
  "contextaction.cc"
  "hash.cc"
  "httpquery.cc"
  "httpresponse.cc"
  "main.cc"
  "mongoose.c"
  "options.cc"
  "searchaction.cc"
  "server.cc"
  )

add_executable(simplifyd ${SIMPLIFYD_SOURCES})
target_link_libraries(simplifyd simplify)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  target_link_libraries(simplifyd stdc++fs)
endif ()

set_property(TARGET simplify PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET simplifyd PROPERTY CXX_STANDARD 17)

install(TARGETS simplifyd RUNTIME DESTINATION bin)
install(DIRECTORY html DESTINATION share/simplify)