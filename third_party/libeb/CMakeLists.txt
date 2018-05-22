find_package(ZLIB REQUIRED)

include_directories(SYSTEM ${ZLIB_INCLUDE_DIRS})
add_definitions(-DEB_BUILD_LIBRARY)

set(LIBEB_SOURCES
  "eb/appendix.c"
  "eb/appsub.c"
  "eb/bcd.c"
  "eb/binary.c"
  "eb/bitmap.c"
  "eb/book.c"
  "eb/booklist.c"
  "eb/copyright.c"
  "eb/cross.c"
  "eb/eb.c"
  "eb/endword.c"
  "eb/error.c"
  "eb/exactword.c"
  "eb/filename.c"
  "eb/font.c"
  "eb/hook.c"
  "eb/jacode.c"
  "eb/keyword.c"
  "eb/lock.c"
  "eb/log.c"
  "eb/match.c"
  "eb/menu.c"
  "eb/multi.c"
  "eb/narwalt.c"
  "eb/narwfont.c"
  "eb/readtext.c"
  "eb/search.c"
  "eb/setword.c"
  "eb/stopcode.c"
  "eb/strcasecmp.c"
  "eb/subbook.c"
  "eb/text.c"
  "eb/widealt.c"
  "eb/widefont.c"
  "eb/word.c"
  "eb/zio.c"
  )

add_library(eb STATIC ${LIBEB_SOURCES})
target_link_libraries(eb ${ZLIB_LIBRARIES})
