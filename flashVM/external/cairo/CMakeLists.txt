# Pinned upstream core source list; no optional GUI/font dependencies.
set(FVM_CAIRO_SOURCES
  upstream/src/cairo-analysis-surface.c
  upstream/src/cairo-arc.c
  upstream/src/cairo-array.c
  upstream/src/cairo-atomic.c
  upstream/src/cairo-base64-stream.c
  upstream/src/cairo-base85-stream.c
  upstream/src/cairo-bentley-ottmann-rectangular.c
  upstream/src/cairo-bentley-ottmann-rectilinear.c
  upstream/src/cairo-bentley-ottmann.c
  upstream/src/cairo-botor-scan-converter.c
  upstream/src/cairo-boxes-intersect.c
  upstream/src/cairo-boxes.c
  upstream/src/cairo-cache.c
  upstream/src/cairo-clip-boxes.c
  upstream/src/cairo-clip-polygon.c
  upstream/src/cairo-clip-region.c
  upstream/src/cairo-clip-surface.c
  upstream/src/cairo-clip-tor-scan-converter.c
  upstream/src/cairo-clip.c
  upstream/src/cairo-color.c
  upstream/src/cairo-composite-rectangles.c
  upstream/src/cairo-compositor.c
  upstream/src/cairo-contour.c
  upstream/src/cairo-damage.c
  upstream/src/cairo-debug.c
  upstream/src/cairo-default-context.c
  upstream/src/cairo-device.c
  upstream/src/cairo-error.c
  upstream/src/cairo-fallback-compositor.c
  upstream/src/cairo-fixed.c
  upstream/src/cairo-font-face-twin-data.c
  upstream/src/cairo-font-face-twin.c
  upstream/src/cairo-font-face.c
  upstream/src/cairo-font-options.c
  upstream/src/cairo-freed-pool.c
  upstream/src/cairo-freelist.c
  upstream/src/cairo-gstate.c
  upstream/src/cairo-hash.c
  upstream/src/cairo-hull.c
  upstream/src/cairo-image-compositor.c
  upstream/src/cairo-image-info.c
  upstream/src/cairo-image-source.c
  upstream/src/cairo-image-surface.c
  upstream/src/cairo-line.c
  upstream/src/cairo-lzw.c
  upstream/src/cairo-mask-compositor.c
  upstream/src/cairo-matrix.c
  upstream/src/cairo-mempool.c
  upstream/src/cairo-mesh-pattern-rasterizer.c
  upstream/src/cairo-misc.c
  upstream/src/cairo-mono-scan-converter.c
  upstream/src/cairo-mutex.c
  upstream/src/cairo-no-compositor.c
  upstream/src/cairo-observer.c
  upstream/src/cairo-output-stream.c
  upstream/src/cairo-paginated-surface.c
  upstream/src/cairo-path-bounds.c
  upstream/src/cairo-path-fill.c
  upstream/src/cairo-path-fixed.c
  upstream/src/cairo-path-in-fill.c
  upstream/src/cairo-path-stroke-boxes.c
  upstream/src/cairo-path-stroke-polygon.c
  upstream/src/cairo-path-stroke-traps.c
  upstream/src/cairo-path-stroke-tristrip.c
  upstream/src/cairo-path-stroke.c
  upstream/src/cairo-path.c
  upstream/src/cairo-pattern.c
  upstream/src/cairo-pen.c
  upstream/src/cairo-polygon-intersect.c
  upstream/src/cairo-polygon-reduce.c
  upstream/src/cairo-polygon.c
  upstream/src/cairo-raster-source-pattern.c
  upstream/src/cairo-recording-surface.c
  upstream/src/cairo-rectangle.c
  upstream/src/cairo-rectangular-scan-converter.c
  upstream/src/cairo-region.c
  upstream/src/cairo-rtree.c
  upstream/src/cairo-scaled-font.c
  upstream/src/cairo-shape-mask-compositor.c
  upstream/src/cairo-slope.c
  upstream/src/cairo-spans-compositor.c
  upstream/src/cairo-spans.c
  upstream/src/cairo-spline.c
  upstream/src/cairo-stroke-dash.c
  upstream/src/cairo-stroke-style.c
  upstream/src/cairo-surface-clipper.c
  upstream/src/cairo-surface-fallback.c
  upstream/src/cairo-surface-observer.c
  upstream/src/cairo-surface-offset.c
  upstream/src/cairo-surface-snapshot.c
  upstream/src/cairo-surface-subsurface.c
  upstream/src/cairo-surface-wrapper.c
  upstream/src/cairo-surface.c
  upstream/src/cairo-time.c
  upstream/src/cairo-tor-scan-converter.c
  upstream/src/cairo-tor22-scan-converter.c
  upstream/src/cairo-toy-font-face.c
  upstream/src/cairo-traps-compositor.c
  upstream/src/cairo-traps.c
  upstream/src/cairo-tristrip.c
  upstream/src/cairo-unicode.c
  upstream/src/cairo-user-font.c
  upstream/src/cairo-version.c
  upstream/src/cairo-wideint.c
  upstream/src/cairo.c
  upstream/src/cairo-cff-subset.c
  upstream/src/cairo-scaled-font-subsets.c
  upstream/src/cairo-truetype-subset.c
  upstream/src/cairo-type1-fallback.c
  upstream/src/cairo-type1-glyph-names.c
  upstream/src/cairo-type1-subset.c
  upstream/src/cairo-type3-glyph-surface.c
  upstream/src/cairo-pdf-operators.c
  upstream/src/cairo-pdf-shading.c
  upstream/src/cairo-tag-attributes.c
  upstream/src/cairo-deflate-stream.c
)
if(FVM_ENABLE_IMAGE_CODECS)
  list(APPEND FVM_CAIRO_SOURCES upstream/src/cairo-png.c upstream/src/cairo-svg-surface.c)
endif()
if(FVM_ENABLE_FONTS)
  list(APPEND FVM_CAIRO_SOURCES upstream/src/cairo-ft-font.c)
endif()
include(CheckTypeSize)
include(TestBigEndian)
check_type_size("long" FVM_VENDOR_SIZEOF_LONG)
test_big_endian(FVM_VENDOR_BIG_ENDIAN)
set(FVM_VENDOR_PTHREAD 1)
if(WIN32)
  set(FVM_VENDOR_PTHREAD 0)
endif()
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
set(FVM_CAIRO_FONTS 0)
if(FVM_ENABLE_FONTS)
  set(FVM_CAIRO_FONTS 1)
endif()
set(FVM_CAIRO_IMAGE_CODECS 0)
if(FVM_ENABLE_IMAGE_CODECS)
  set(FVM_CAIRO_IMAGE_CODECS 1)
endif()
configure_file(cairo-features.h.in ${CMAKE_CURRENT_BINARY_DIR}/cairo-features.h @ONLY)
add_library(fvm_cairo STATIC ${FVM_CAIRO_SOURCES})
target_include_directories(fvm_cairo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/upstream/src ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(fvm_cairo PRIVATE HAVE_CONFIG_H=1)
target_include_directories(fvm_cairo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/upstream)
target_link_libraries(fvm_cairo PUBLIC fvm_pixman fvm_zlib)
if(FVM_ENABLE_IMAGE_CODECS)
  target_link_libraries(fvm_cairo PUBLIC fvm_png)
endif()
if(NOT WIN32)
  target_link_libraries(fvm_cairo PUBLIC Threads::Threads m)
endif()

if(FVM_ENABLE_FONTS)
  target_link_libraries(fvm_cairo PUBLIC fvm_freetype)
endif()
