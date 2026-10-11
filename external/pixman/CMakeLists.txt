# Pinned upstream core source list; no optional GUI/font dependencies.
set(FVM_PIXMAN_SOURCES
  upstream/pixman/pixman.c
  upstream/pixman/pixman-access.c
  upstream/pixman/pixman-access-accessors.c
  upstream/pixman/pixman-bits-image.c
  upstream/pixman/pixman-combine32.c
  upstream/pixman/pixman-combine-float.c
  upstream/pixman/pixman-conical-gradient.c
  upstream/pixman/pixman-filter.c
  upstream/pixman/pixman-x86.c
  upstream/pixman/pixman-mips.c
  upstream/pixman/pixman-arm.c
  upstream/pixman/pixman-ppc.c
  upstream/pixman/pixman-edge.c
  upstream/pixman/pixman-edge-accessors.c
  upstream/pixman/pixman-fast-path.c
  upstream/pixman/pixman-glyph.c
  upstream/pixman/pixman-general.c
  upstream/pixman/pixman-gradient-walker.c
  upstream/pixman/pixman-image.c
  upstream/pixman/pixman-implementation.c
  upstream/pixman/pixman-linear-gradient.c
  upstream/pixman/pixman-matrix.c
  upstream/pixman/pixman-noop.c
  upstream/pixman/pixman-radial-gradient.c
  upstream/pixman/pixman-region16.c
  upstream/pixman/pixman-region32.c
  upstream/pixman/pixman-solid-fill.c
  upstream/pixman/pixman-timer.c
  upstream/pixman/pixman-trap.c
  upstream/pixman/pixman-utils.c
)
include(CheckTypeSize)
include(TestBigEndian)
check_type_size("long" FVM_VENDOR_SIZEOF_LONG)
test_big_endian(FVM_VENDOR_BIG_ENDIAN)
set(FVM_VENDOR_PTHREAD 1)
if(WIN32)
  set(FVM_VENDOR_PTHREAD 0)
endif()
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
set(PIXMAN_VERSION_MAJOR 0)
set(PIXMAN_VERSION_MINOR 38)
set(PIXMAN_VERSION_MICRO 4)
configure_file(upstream/pixman/pixman-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/pixman-version.h @ONLY)
add_library(fvm_pixman STATIC ${FVM_PIXMAN_SOURCES})
target_include_directories(fvm_pixman PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/upstream/pixman ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(fvm_pixman PRIVATE HAVE_CONFIG_H=1)
if(NOT WIN32)
  target_link_libraries(fvm_pixman PUBLIC Threads::Threads m)
endif()
