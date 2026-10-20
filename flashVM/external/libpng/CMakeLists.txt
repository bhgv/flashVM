# Scalar C89 build; use the bundled zlib and the upstream prebuilt configuration.
set(FVM_LIBPNG_SOURCES
  upstream/png.c upstream/pngerror.c upstream/pngget.c upstream/pngmem.c
  upstream/pngpread.c upstream/pngread.c upstream/pngrio.c upstream/pngrtran.c
  upstream/pngrutil.c upstream/pngset.c upstream/pngtrans.c upstream/pngwio.c
  upstream/pngwrite.c upstream/pngwtran.c upstream/pngwutil.c)
add_library(fvm_png STATIC ${FVM_LIBPNG_SOURCES})
target_include_directories(fvm_png PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/upstream)
target_compile_definitions(fvm_png PRIVATE PNG_ARM_NEON_OPT=0 PNG_POWERPC_VSX_OPT=0
  PNG_INTEL_SSE_OPT=0 PNG_MIPS_MSA_OPT=0 PNG_LOONGARCH_LSX_OPT=0)
target_link_libraries(fvm_png PUBLIC fvm_zlib)
if(NOT WIN32)
  target_link_libraries(fvm_png PUBLIC m)
endif()

# Upstream sources use C99 integer/types; keep this private to the vendor target.
set_target_properties(fvm_png PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS OFF)
