set(FVM_LIBUNIBREAK_SOURCES
  upstream/src/unibreakbase.c
  upstream/src/unibreakdef.c
  upstream/src/linebreak.c
  upstream/src/linebreakdata.c
  upstream/src/linebreakdef.c
  upstream/src/eastasianwidthdef.c
  upstream/src/emojidef.c
  upstream/src/graphemebreak.c
  upstream/src/wordbreak.c
)
add_library(fvm_unibreak STATIC ${FVM_LIBUNIBREAK_SOURCES})
target_include_directories(fvm_unibreak PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/upstream/src)
