set(FVM_FRIBIDI_SOURCES
  upstream/lib/fribidi-arabic.c
  upstream/lib/fribidi-bidi-types.c
  upstream/lib/fribidi-bidi.c
  upstream/lib/fribidi-brackets.c
  upstream/lib/fribidi-char-sets-cap-rtl.c
  upstream/lib/fribidi-char-sets-cp1255.c
  upstream/lib/fribidi-char-sets-cp1256.c
  upstream/lib/fribidi-char-sets-iso8859-6.c
  upstream/lib/fribidi-char-sets-iso8859-8.c
  upstream/lib/fribidi-char-sets-utf8.c
  upstream/lib/fribidi-char-sets.c
  upstream/lib/fribidi-deprecated.c
  upstream/lib/fribidi-joining-types.c
  upstream/lib/fribidi-joining.c
  upstream/lib/fribidi-mirroring.c
  upstream/lib/fribidi-run.c
  upstream/lib/fribidi-shape.c
  upstream/lib/fribidi.c
)
add_library(fvm_fribidi STATIC ${FVM_FRIBIDI_SOURCES})
target_include_directories(fvm_fribidi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/upstream/lib ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(fvm_fribidi PUBLIC FRIBIDI_LIB_STATIC PRIVATE FRIBIDI_BUILD HAVE_CONFIG_H=1)
