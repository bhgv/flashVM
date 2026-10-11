set(FVM_FREETYPE_SOURCES
  upstream/src/autofit/autofit.c
  upstream/src/base/ftbase.c
  upstream/src/base/ftbbox.c
  upstream/src/base/ftbdf.c
  upstream/src/base/ftbitmap.c
  upstream/src/base/ftcid.c
  upstream/src/base/ftfstype.c
  upstream/src/base/ftgasp.c
  upstream/src/base/ftglyph.c
  upstream/src/base/ftgxval.c
  upstream/src/base/ftinit.c
  upstream/src/base/ftmm.c
  upstream/src/base/ftotval.c
  upstream/src/base/ftpatent.c
  upstream/src/base/ftpfr.c
  upstream/src/base/ftstroke.c
  upstream/src/base/ftsynth.c
  upstream/src/base/fttype1.c
  upstream/src/base/ftwinfnt.c
  upstream/src/bdf/bdf.c
  upstream/src/bzip2/ftbzip2.c
  upstream/src/cache/ftcache.c
  upstream/src/cff/cff.c
  upstream/src/cid/type1cid.c
  upstream/src/gzip/ftgzip.c
  upstream/src/lzw/ftlzw.c
  upstream/src/pcf/pcf.c
  upstream/src/pfr/pfr.c
  upstream/src/psaux/psaux.c
  upstream/src/pshinter/pshinter.c
  upstream/src/psnames/psnames.c
  upstream/src/raster/raster.c
  upstream/src/sdf/sdf.c
  upstream/src/sfnt/sfnt.c
  upstream/src/smooth/smooth.c
  upstream/src/svg/svg.c
  upstream/src/truetype/truetype.c
  upstream/src/type1/type1.c
  upstream/src/type42/type42.c
  upstream/src/winfonts/winfnt.c
  upstream/src/base/ftsystem.c
  upstream/src/base/ftdebug.c
)
add_library(fvm_freetype STATIC ${FVM_FREETYPE_SOURCES})
target_include_directories(fvm_freetype PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/upstream/include)
target_compile_definitions(fvm_freetype PRIVATE FT2_BUILD_LIBRARY FT_CONFIG_OPTION_SYSTEM_ZLIB)
target_link_libraries(fvm_freetype PUBLIC fvm_zlib)
