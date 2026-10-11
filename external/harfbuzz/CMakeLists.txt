set(FVM_HARFBUZZ_SOURCES
  upstream/src/hb-blob.cc
  upstream/src/hb-buffer-serialize.cc
  upstream/src/hb-buffer.cc
  upstream/src/hb-common.cc
  upstream/src/hb-face.cc
  upstream/src/hb-font.cc
  upstream/src/hb-map.cc
  upstream/src/hb-ot-tag.cc
  upstream/src/hb-set.cc
  upstream/src/hb-shape.cc
  upstream/src/hb-shape-plan.cc
  upstream/src/hb-shaper.cc
  upstream/src/hb-static.cc
  upstream/src/hb-unicode.cc
  upstream/src/hb-warning.cc
  upstream/src/hb-fallback-shape.cc
  upstream/src/hb-aat-layout.cc
  upstream/src/hb-ot-font.cc
  upstream/src/hb-ot-layout.cc
  upstream/src/hb-ot-color.cc
  upstream/src/hb-ot-map.cc
  upstream/src/hb-ot-math.cc
  upstream/src/hb-ot-shape.cc
  upstream/src/hb-ot-shape-complex-arabic.cc
  upstream/src/hb-ot-shape-complex-default.cc
  upstream/src/hb-ot-shape-complex-hangul.cc
  upstream/src/hb-ot-shape-complex-hebrew.cc
  upstream/src/hb-ot-shape-complex-indic.cc
  upstream/src/hb-ot-shape-complex-indic-table.cc
  upstream/src/hb-ot-shape-complex-khmer.cc
  upstream/src/hb-ot-shape-complex-myanmar.cc
  upstream/src/hb-ot-shape-complex-thai.cc
  upstream/src/hb-ot-shape-complex-tibetan.cc
  upstream/src/hb-ot-shape-complex-use.cc
  upstream/src/hb-ot-shape-complex-use-table.cc
  upstream/src/hb-ot-shape-normalize.cc
  upstream/src/hb-ot-shape-fallback.cc
  upstream/src/hb-ot-var.cc
  upstream/src/hb-ucdn.cc
  upstream/src/hb-ft.cc
  upstream/src/hb-ucdn/ucdn.c
)
add_library(fvm_harfbuzz STATIC ${FVM_HARFBUZZ_SOURCES})
target_include_directories(fvm_harfbuzz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/upstream/src PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/upstream/src/hb-ucdn)
target_compile_definitions(fvm_harfbuzz PRIVATE HAVE_ATEXIT=1 HAVE_FREETYPE=1 HAVE_OT=1 HAVE_FALLBACK=1 HAVE_UCDN=1 HB_NO_WIN1256=1)
target_link_libraries(fvm_harfbuzz PUBLIC fvm_freetype)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(fvm_harfbuzz PRIVATE -fno-strict-aliasing)
endif()

if(NOT WIN32)
  target_compile_definitions(fvm_harfbuzz PRIVATE HAVE_PTHREAD=1)
  target_link_libraries(fvm_harfbuzz PUBLIC Threads::Threads)
endif()
