# Temporary native Limbo bootstrap from selected owned upstream sources.
cmake_minimum_required(VERSION 3.10)
project(limbo_bootstrap VERSION 0.1.0 LANGUAGES C)
if(NOT UNIX)
  message(FATAL_ERROR "This initial host build supports POSIX; native OS adapters remain separate")
endif()
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-fno-strict-aliasing)
endif()
set(u "${CMAKE_CURRENT_SOURCE_DIR}/upstream")
set(host_include "${CMAKE_CURRENT_SOURCE_DIR}/host/include")
# The upstream lib9 header is shared POSIX C declarations; ARM assembly is never built.
set(sdk_include "${host_include}" "${u}/Linux/arm/include" "${u}/include" "${u}/libinterp")

# Keep legacy modules unchanged; an explicit experimental target matches native cells.
set(LIMBO_BOOTSTRAP_CELL_BYTES "4" CACHE STRING "Dis target cell bytes: legacy 4 or experimental native 8")
set_property(CACHE LIMBO_BOOTSTRAP_CELL_BYTES PROPERTY STRINGS 4 8)
if(NOT LIMBO_BOOTSTRAP_CELL_BYTES MATCHES "^(4|8)$")
  message(FATAL_ERROR "LIMBO_BOOTSTRAP_CELL_BYTES must be 4 or 8")
endif()
# Python is a source-preparation tool only; no Python is deployed with the runtime.
find_package(PythonInterp 3 REQUIRED)
set(limbo_ecom "${CMAKE_CURRENT_BINARY_DIR}/prepared-ecom.c")
execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_ecom.py"
  --source "${u}/limbo/ecom.c" --output "${limbo_ecom}" --cells "${LIMBO_BOOTSTRAP_CELL_BYTES}"
  RESULT_VARIABLE ecom_prepared)
if(NOT ecom_prepared EQUAL 0)
  message(FATAL_ERROR "Limbo constant/source preparation failed")
endif()
set(limbo_dtoa "${CMAKE_CURRENT_BINARY_DIR}/prepared-dtoa.c")
execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_dtoa.py"
  --source "${u}/libmath/dtoa.c" --output "${limbo_dtoa}" RESULT_VARIABLE dtoa_prepared)
if(NOT dtoa_prepared EQUAL 0)
  message(FATAL_ERROR "Limbo dtoa source preparation failed")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/prepare_ecom.py" "${u}/limbo/ecom.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/prepare_dtoa.py" "${u}/libmath/dtoa.c")
if(LIMBO_BOOTSTRAP_CELL_BYTES STREQUAL "8")
  file(READ "${u}/include/isa.h" target_isa)
  string(REPLACE "IBY2WD\t= 4" "IBY2WD\t= 8" target_isa "${target_isa}")
  string(REPLACE "XMAGIC\t= 819248" "XMAGIC\t= 819264" target_isa "${target_isa}")
  string(REPLACE "SMAGIC\t= 923426" "SMAGIC\t= 923442" target_isa "${target_isa}")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/target-include")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/target-include/isa.h" "${target_isa}")
  list(INSERT sdk_include 0 "${CMAKE_CURRENT_BINARY_DIR}/target-include")
endif()

set(target_magic 819248)
if(LIMBO_BOOTSTRAP_CELL_BYTES STREQUAL "8")
  set(target_magic 819264)
endif()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/target-abi.json"
  "{\"format\":\"Dis\",\"cell_bytes\":${LIMBO_BOOTSTRAP_CELL_BYTES},\"magic\":${target_magic}}\n")

add_library(limbo_bootstrap_lib9 STATIC
  upstream/lib9/convD2M.c
  upstream/lib9/convM2D.c
  upstream/lib9/convM2S.c
  upstream/lib9/convS2M.c
  upstream/lib9/fcallfmt.c
  upstream/lib9/runestrchr.c
  upstream/lib9/runestrlen.c
  upstream/lib9/runetype.c
  upstream/lib9/strtoll.c
  upstream/lib9/strtoull.c
  upstream/lib9/rune.c
  upstream/lib9/argv0.c
  upstream/lib9/charstod.c
  upstream/lib9/cistrcmp.c
  upstream/lib9/cistrncmp.c
  upstream/lib9/cistrstr.c
  upstream/lib9/cleanname.c
  upstream/lib9/create.c
  upstream/lib9/dirwstat.c
  upstream/lib9/dofmt.c
  upstream/lib9/dorfmt.c
  upstream/lib9/errfmt.c
  upstream/lib9/exits.c
  upstream/lib9/fmt.c
  upstream/lib9/fmtfd.c
  upstream/lib9/fmtlock.c
  upstream/lib9/fmtprint.c
  upstream/lib9/fmtquote.c
  upstream/lib9/fmtrune.c
  upstream/lib9/fmtstr.c
  upstream/lib9/fmtvprint.c
  upstream/lib9/fprint.c
  upstream/lib9/getfields.c
  upstream/lib9/nulldir.c
  upstream/lib9/pow10.c
  upstream/lib9/print.c
  upstream/lib9/qsort.c
  upstream/lib9/readn.c
  upstream/lib9/rerrstr.c
  upstream/lib9/runeseprint.c
  upstream/lib9/runesmprint.c
  upstream/lib9/runesnprint.c
  upstream/lib9/runevseprint.c
  upstream/lib9/seek.c
  upstream/lib9/seprint.c
  upstream/lib9/smprint.c
  upstream/lib9/snprint.c
  upstream/lib9/sprint.c
  upstream/lib9/strdup.c
  upstream/lib9/strecpy.c
  upstream/lib9/sysfatal.c
  upstream/lib9/tokenize.c
  upstream/lib9/u16.c
  upstream/lib9/u32.c
  upstream/lib9/u64.c
  upstream/lib9/utflen.c
  upstream/lib9/utfnlen.c
  upstream/lib9/utfrrune.c
  upstream/lib9/utfrune.c
  upstream/lib9/utfecpy.c
  upstream/lib9/vfprint.c
  upstream/lib9/vseprint.c
  upstream/lib9/vsmprint.c
  upstream/lib9/vsnprint.c
  upstream/lib9/dirstat-posix.c
  upstream/lib9/errstr-posix.c
  upstream/lib9/getuser-posix.c
  upstream/lib9/getwd-posix.c
  upstream/lib9/sbrk-posix.c
  upstream/lib9/isnan-posix.c
)
target_include_directories(limbo_bootstrap_lib9 PUBLIC ${sdk_include})
add_library(limbo_bootstrap_bio STATIC
  upstream/libbio/bbuffered.c
  upstream/libbio/bfildes.c
  upstream/libbio/bflush.c
  upstream/libbio/bgetc.c
  upstream/libbio/bgetd.c
  upstream/libbio/bgetrune.c
  upstream/libbio/binit.c
  upstream/libbio/boffset.c
  upstream/libbio/bprint.c
  upstream/libbio/bputc.c
  upstream/libbio/bputrune.c
  upstream/libbio/brdline.c
  upstream/libbio/brdstr.c
  upstream/libbio/bread.c
  upstream/libbio/bseek.c
  upstream/libbio/bvprint.c
  upstream/libbio/bwrite.c
)
target_link_libraries(limbo_bootstrap_bio PUBLIC limbo_bootstrap_lib9)
add_library(limbo_bootstrap_support STATIC host/bootstrap_support.c
  upstream/libmath/FPcontrol-Linux.c "${limbo_dtoa}" upstream/libmath/g_fmt.c upstream/libmath/gfltconv.c
  upstream/libsec/port/md5.c upstream/libsec/port/md5block.c)
target_link_libraries(limbo_bootstrap_support PUBLIC limbo_bootstrap_lib9 m)
# Optional owner-supplied yacc regeneration; default standalone builds can still
# consume committed bootstrap artifacts. New project grammars remain Coco/R.
set(IYACC_EXECUTABLE "" CACHE FILEPATH "Optional source-built Inferno iyacc")
set(IYACC_ROOT "" CACHE PATH "iyacc skeleton root, containing utils/lib/yaccpar")
set(limbo_parser "${u}/limbo/y.tab.c")
set(limbo_lexer "${u}/limbo/lex.c")
if(IYACC_EXECUTABLE)
  if(NOT EXISTS "${IYACC_EXECUTABLE}" OR NOT EXISTS "${IYACC_ROOT}/utils/lib/yaccpar")
    message(FATAL_ERROR "Provide matching IYACC_EXECUTABLE and IYACC_ROOT")
  endif()
  set(parser_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
  file(MAKE_DIRECTORY "${parser_dir}")
  configure_file("${u}/limbo/lex.c" "${parser_dir}/lex.c" COPYONLY)
  set(limbo_lexer "${parser_dir}/lex.c")
  set(limbo_parser "${parser_dir}/y.tab.c")
  add_custom_command(OUTPUT "${limbo_parser}" "${parser_dir}/y.tab.h"
    COMMAND "${CMAKE_COMMAND}" -E copy "${u}/limbo/limbo.y" "${parser_dir}/limbo.y"
    COMMAND "${CMAKE_COMMAND}" -E env "ROOT=${IYACC_ROOT}" "${IYACC_EXECUTABLE}" -d limbo.y
    WORKING_DIRECTORY "${parser_dir}"
    DEPENDS "${u}/limbo/limbo.y" "${IYACC_EXECUTABLE}" "${IYACC_ROOT}/utils/lib/yaccpar" VERBATIM)
  # The copied lexer includes the newly generated token header from its own directory.
  set_source_files_properties("${limbo_lexer}" PROPERTIES OBJECT_DEPENDS "${parser_dir}/y.tab.h")
endif()
# Preserve all grammar actions; reserve one real sentinel instead of forming yys[-1].
set(raw_limbo_parser "${limbo_parser}")
set(limbo_parser "${CMAKE_CURRENT_BINARY_DIR}/safe-parser.c")
add_custom_command(OUTPUT "${limbo_parser}"
  COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_parser.py"
    --source "${raw_limbo_parser}" --output "${limbo_parser}"
  DEPENDS "${raw_limbo_parser}" prepare_parser.py VERBATIM)
add_executable(limbo-bootstrap
  upstream/limbo/asm.c
  upstream/limbo/com.c
  upstream/limbo/decls.c
  upstream/limbo/dis.c
  upstream/limbo/dtocanon.c
  "${limbo_ecom}"
  upstream/limbo/gen.c
  "${limbo_lexer}"
  upstream/limbo/nodes.c
  upstream/limbo/optab.c
  upstream/limbo/optim.c
  upstream/limbo/sbl.c
  upstream/limbo/stubs.c
  upstream/limbo/typecheck.c
  upstream/limbo/types.c
  "${limbo_parser}"
)
target_include_directories(limbo-bootstrap PRIVATE "${u}/limbo")
target_compile_definitions(limbo-bootstrap PRIVATE "INCPATH=\"${u}/module\"")
# Repeat support archives to resolve the old mutually dependent lib9/math helpers.
target_link_libraries(limbo-bootstrap PRIVATE limbo_bootstrap_bio limbo_bootstrap_support limbo_bootstrap_lib9 limbo_bootstrap_support m)

include(CTest)
if(BUILD_TESTING)
  # FindPython3 starts at CMake 3.12; retain standalone 3.10 builds.
  if(CMAKE_VERSION VERSION_LESS "3.12")
    find_package(PythonInterp 3 REQUIRED)
    set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
    set(Python3_Interpreter_FOUND "${PYTHONINTERP_FOUND}")
  else()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
  endif()
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tmp")
  add_test(NAME limbo_bootstrap_products COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.py" $<TARGET_FILE:limbo-bootstrap>)
  set_tests_properties(limbo_bootstrap_products PROPERTIES TIMEOUT 60
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

if(BUILD_TESTING)
  add_test(NAME limbo_bootstrap_big_constant_pool COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/big_constants.py" $<TARGET_FILE:limbo-bootstrap>)
  set_tests_properties(limbo_bootstrap_big_constant_pool PROPERTIES TIMEOUT 30
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Exercise production optimizer range guards in both bytecode cell models.
if(BUILD_TESTING)
  add_test(NAME limbo_bootstrap_optimizer_ranges COMMAND "${Python3_EXECUTABLE}"
    "${u}/limbo/tests/optimizer-ranges.py")
  set_tests_properties(limbo_bootstrap_optimizer_ranges PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Imported native-width formatter span guards use production lib9 formatters.
if(BUILD_TESTING)
  add_test(NAME limbo_bootstrap_native_output_spans COMMAND "${Python3_EXECUTABLE}"
    "${u}/lib9/tests/native-output-spans.py")
  # The retained neutral POSIX lib9 declarations live under Linux/arm here.
  set_tests_properties(limbo_bootstrap_native_output_spans PROPERTIES TIMEOUT 60
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};CFLAGS=-I${u}/Linux/arm/include -O1 -g -DNDEBUG -fsanitize=address,undefined -fno-sanitize-recover=all;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Verify the selected declaration/label/occurrence traversals, including both deep associations.
if(BUILD_TESTING)
  add_test(NAME limbo_bootstrap_node_chains COMMAND "${Python3_EXECUTABLE}"
    "${u}/limbo/tests/node-chains.py")
  set_tests_properties(limbo_bootstrap_node_chains PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

if(BUILD_TESTING)
  add_test(NAME limbo_bootstrap_node_queries COMMAND "${Python3_EXECUTABLE}"
    "${u}/limbo/tests/node-queries.py")
  set_tests_properties(limbo_bootstrap_node_queries PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  add_test(NAME limbo_bootstrap_node_transforms COMMAND "${Python3_EXECUTABLE}"
    "${u}/limbo/tests/node-transforms.py")
  set_tests_properties(limbo_bootstrap_node_transforms PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  add_test(NAME limbo_bootstrap_inline_graphs COMMAND "${Python3_EXECUTABLE}"
    "${u}/limbo/tests/inline-graphs.py")
  set_tests_properties(limbo_bootstrap_inline_graphs PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Exercise real Bio pushback/count/position boundaries used by the source compiler.
if(BUILD_TESTING)
  add_test(NAME limbo_bootstrap_bio_boundaries COMMAND "${Python3_EXECUTABLE}"
    "${u}/libbio/tests/boundaries.py")
  set_tests_properties(limbo_bootstrap_bio_boundaries PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Link real format dispatch to deterministic native-write/state fixtures.
if(BUILD_TESTING)
  add_test(NAME limbo_bootstrap_bio_print_state COMMAND "${Python3_EXECUTABLE}"
    "${u}/libbio/tests/print-state.py")
  set_tests_properties(limbo_bootstrap_bio_print_state PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Check complete unread windows and moving ownership in the selected source libraries.
if(BUILD_TESTING)
  add_test(NAME limbo_bootstrap_bio_refill_windows COMMAND "${Python3_EXECUTABLE}"
    "${u}/libbio/tests/refill-windows.py")
  set_tests_properties(limbo_bootstrap_bio_refill_windows PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  add_test(NAME limbo_bootstrap_allocated_format COMMAND "${Python3_EXECUTABLE}"
    "${u}/lib9/tests/allocated-format.py")
  set_tests_properties(limbo_bootstrap_allocated_format PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};ALLOC_FMT_LIBRARIES=lib9;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Resolve public Rune wrappers against the actual archive, not text-included test bodies.
if(BUILD_TESTING)
  add_executable(limbo_bootstrap_rune_providers tests/rune_providers.c)
  target_link_libraries(limbo_bootstrap_rune_providers PRIVATE limbo_bootstrap_lib9)
  add_test(NAME limbo_bootstrap_rune_providers COMMAND limbo_bootstrap_rune_providers)
endif()

# Compare ordered diagnostic output with a shallow oracle and bound deep AST traversal.
if(BUILD_TESTING)
  add_test(NAME limbo_bootstrap_diagnostic_print COMMAND "${Python3_EXECUTABLE}"
    "${u}/limbo/tests/diagnostic-print.py")
  set_tests_properties(limbo_bootstrap_diagnostic_print PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Verify pointer maps and zero-initialization walks for classic and native slot widths.
if(BUILD_TESTING)
  add_test(NAME limbo_bootstrap_type_walks COMMAND "${Python3_EXECUTABLE}"
    "${u}/limbo/tests/type-walks.py")
  set_tests_properties(limbo_bootstrap_type_walks PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  add_test(NAME limbo_bootstrap_descriptor_maps COMMAND "${Python3_EXECUTABLE}"
    "${u}/limbo/tests/descriptor-maps.py")
  set_tests_properties(limbo_bootstrap_descriptor_maps PROPERTIES TIMEOUT 120
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()

# Retain deep traversal and native-width metadata acceptance from experiment.
if(BUILD_TESTING)
  foreach(driver type-parent type-print stub-print declaration-walks reference-sort reference-uses comm-alias-walks scope-binding)
    add_test(NAME limbo_bootstrap_${driver} COMMAND "${Python3_EXECUTABLE}"
      "${u}/limbo/tests/${driver}.py")
    set_tests_properties(limbo_bootstrap_${driver} PROPERTIES TIMEOUT 180
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;ASAN_OPTIONS=detect_leaks=0")
  endforeach()
endif()
