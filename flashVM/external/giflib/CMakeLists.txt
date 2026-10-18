set(FVM_GIFLIB_SOURCES
  upstream/dgif_lib.c upstream/egif_lib.c upstream/gifalloc.c upstream/gif_err.c
  upstream/gif_font.c upstream/gif_hash.c upstream/openbsd-reallocarray.c
  upstream/quantize.c)
add_library(fvm_gif STATIC ${FVM_GIFLIB_SOURCES})
target_include_directories(fvm_gif PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/upstream)
if(NOT WIN32)
  target_compile_definitions(fvm_gif PRIVATE _POSIX_C_SOURCE=200809L)
endif()
