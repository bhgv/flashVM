cmake_minimum_required(VERSION 3.10)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" FVM_VERSION LIMIT_COUNT 1)
project(FlashVM VERSION "${FVM_VERSION}" LANGUAGES C CXX)

# The VM and bundled backends must build without modern C/C++ language modes.
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -pedantic-errors
    $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration>)
endif()

option(FVM_BUILD_TESTS "Build regression tests" ON)
option(FVM_BUILD_EXAMPLES "Build embedding and extension examples" ON)
option(FVM_BUILD_REFERENCE_RUNNER "Build the SWF action/root-timeline trace comparison runner" OFF)
include(cmake/BuildDefinitions.cmake)
option(FVM_ENABLE_RENDER "Require a raster backend" OFF)
if(FVM_ENABLE_RENDER AND NOT FVM_ENABLE_AGG AND NOT FVM_ENABLE_CAIRO)
  message(FATAL_ERROR "FVM_ENABLE_RENDER requires GRAPHICS_AGG or GRAPHICS_CAIRO in DEFS.txt")
endif()

file(GLOB_RECURSE FVM_CORE_SOURCES src/core/*.cpp src/capi/*.cpp src/capi/*.c
                                 src/avm1/*.cpp src/swf/*.cpp src/standard/*.cpp src/jit/*.cpp)
list(REMOVE_ITEM FVM_CORE_SOURCES ${PROJECT_SOURCE_DIR}/src/capi/sqlite_c.cpp)
add_library(flashvm STATIC ${FVM_CORE_SOURCES} src/render/raster_c.cpp
  ports/code_port.c ports/file_port.c ports/calendar_port.c ports/${FVM_PORT}/clock.c ports/${FVM_PORT}/monotonic.c ports/${FVM_PORT}/thread.c ports/${FVM_PORT}/channel_sync.c
  external/mini_regex/mini_regex.c)
target_include_directories(flashvm PUBLIC ${PROJECT_SOURCE_DIR}/include ${FVM_GENERATED_INCLUDE}
  PRIVATE ${PROJECT_SOURCE_DIR}/ports ${PROJECT_SOURCE_DIR}/external/mini_regex)
target_compile_definitions(flashvm PUBLIC ${FVM_BUILD_KEYS})
if(NOT WIN32)
  find_package(Threads REQUIRED)
  target_link_libraries(flashvm PUBLIC Threads::Threads m)
endif()
if(FVM_ENABLE_RENDER_JOBS)
  target_sources(flashvm PRIVATE src/render/render_jobs.c)
endif()
if(FVM_ENABLE_OS_PORTS)
  target_sources(flashvm PRIVATE ports/${FVM_PORT}/input.c ports/${FVM_PORT}/network.c)
  if(WIN32)
    target_link_libraries(flashvm PUBLIC ws2_32)
  elseif(ANDROID)
    target_link_libraries(flashvm PUBLIC android)
  elseif(APPLE)
    find_library(FVM_APPLICATION_SERVICES ApplicationServices)
    if(NOT FVM_APPLICATION_SERVICES)
      message(FATAL_ERROR "ApplicationServices is required by the macOS input port")
    endif()
    target_link_libraries(flashvm PUBLIC ${FVM_APPLICATION_SERVICES})
  endif()
endif()

# Compile real pinned dependency sources; no downloads during configuration.
set(FVM_ZLIB_DIR ${PROJECT_SOURCE_DIR}/external/zlib/upstream)
add_library(fvm_zlib STATIC
  ${FVM_ZLIB_DIR}/adler32.c ${FVM_ZLIB_DIR}/crc32.c
  ${FVM_ZLIB_DIR}/deflate.c ${FVM_ZLIB_DIR}/trees.c
  ${FVM_ZLIB_DIR}/inflate.c ${FVM_ZLIB_DIR}/inftrees.c
  ${FVM_ZLIB_DIR}/inffast.c ${FVM_ZLIB_DIR}/zutil.c
  ${FVM_ZLIB_DIR}/compress.c ${FVM_ZLIB_DIR}/uncompr.c)
target_include_directories(fvm_zlib PUBLIC ${FVM_ZLIB_DIR})
target_link_libraries(flashvm PUBLIC fvm_zlib)
if(FVM_ENABLE_AGG)
  file(GLOB FVM_AGG_SOURCES external/agg/upstream/src/*.cpp)
  add_library(fvm_agg STATIC ${FVM_AGG_SOURCES})
  target_include_directories(fvm_agg PUBLIC ${PROJECT_SOURCE_DIR}/external/agg/upstream/include)
  if(FVM_ENABLE_FONTS)
    target_sources(fvm_agg PRIVATE external/agg/upstream/font_freetype/agg_font_freetype.cpp)
    target_include_directories(fvm_agg PUBLIC ${PROJECT_SOURCE_DIR}/external/agg/upstream/font_freetype)
    target_link_libraries(fvm_agg PUBLIC fvm_freetype)
  endif()
  target_link_libraries(flashvm PUBLIC fvm_agg)
  target_compile_definitions(flashvm PUBLIC FVM_HAS_AGG=1)
endif()
# Font libraries are pinned, static and available to native extension clients.
if(FVM_ENABLE_FONTS)
  add_subdirectory(external/freetype)
  add_subdirectory(external/harfbuzz)
  add_subdirectory(external/libunibreak)
  add_subdirectory(external/fribidi)
  target_link_libraries(flashvm PUBLIC fvm_harfbuzz fvm_unibreak fvm_fribidi)
  target_compile_definitions(flashvm PUBLIC FVM_HAS_FONTS=1)
endif()
# Image libraries are public dependencies: project/extension sources can use
# their headers/APIs through the flashvm target without external installations.
if(FVM_ENABLE_IMAGE_CODECS)
  add_subdirectory(external/libpng)
  add_subdirectory(external/giflib)
  add_subdirectory(external/libjpeg)
  add_subdirectory(external/nanosvg)
  target_link_libraries(flashvm PUBLIC fvm_png fvm_gif fvm_jpeg fvm_nanosvg)
  target_compile_definitions(flashvm PUBLIC FVM_HAS_IMAGE_CODECS=1)
endif()
# Cairo is also the SVG writer in AGG/headless builds. The rasterizer choice
# remains independent: only GRAPHICS_CAIRO enables FVM_HAS_CAIRO.
if(FVM_ENABLE_CAIRO OR FVM_ENABLE_IMAGE_CODECS)
  add_subdirectory(external/pixman)
  add_subdirectory(external/cairo)
  target_link_libraries(flashvm PUBLIC fvm_cairo)
endif()
if(FVM_ENABLE_CAIRO)
  target_compile_definitions(flashvm PUBLIC FVM_HAS_CAIRO=1)
endif()
if(FVM_ENABLE_SQLITE)
  add_subdirectory(external/sqlite)
  target_sources(flashvm PRIVATE src/capi/sqlite_c.cpp)
  target_link_libraries(flashvm PUBLIC fvm_sqlite)
  target_compile_definitions(flashvm PUBLIC FVM_HAS_SQLITE=1)
endif()

# A host BUILD_SHARED_LIBS setting must never change bundled dependencies to DLLs.
foreach(dependency flashvm fvm_zlib fvm_agg fvm_png fvm_gif fvm_jpeg fvm_nanosvg
                   fvm_cairo fvm_pixman fvm_sqlite fvm_freetype fvm_harfbuzz
                   fvm_unibreak fvm_fribidi)
  if(TARGET ${dependency})
    get_target_property(dependency_type ${dependency} TYPE)
    if(NOT dependency_type STREQUAL "STATIC_LIBRARY")
      message(FATAL_ERROR "Bundled target ${dependency} must be static")
    endif()
  endif()
endforeach()

# Use one registration path so the CMake suite cannot silently omit new tests.
if(FVM_BUILD_TESTS)
  enable_testing()
endif()
function(fvm_add_program source)
  file(RELATIVE_PATH relative ${PROJECT_SOURCE_DIR} ${source})
  if(relative MATCHES "font_libraries" AND NOT FVM_ENABLE_FONTS)
    return()
  endif()
  if(relative MATCHES "image_codecs" AND NOT FVM_ENABLE_IMAGE_CODECS)
    return()
  endif()
  if(relative MATCHES "sqlite" AND NOT FVM_ENABLE_SQLITE)
    return()
  endif()
  if(relative MATCHES "os_ports" AND NOT FVM_ENABLE_OS_PORTS)
    return()
  endif()
  if(relative MATCHES "(render_jobs|parallel_render)" AND NOT FVM_ENABLE_RENDER_JOBS)
    return()
  endif()
  string(REGEX REPLACE "\\.[^.]+$" "" name ${relative})
  string(REPLACE "/" "_" name ${name})
  set(target flashvm_${name})
  add_executable(${target} ${source})
  target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/ports)
  target_link_libraries(${target} PRIVATE flashvm)
  # Keep assertions active in test/example executables, including Release.
  if(MSVC)
    target_compile_options(${target} PRIVATE /UNDEBUG)
  else()
    target_compile_options(${target} PRIVATE -UNDEBUG)
  endif()
  if(FVM_BUILD_TESTS)
    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES TIMEOUT 30)
    if(target STREQUAL "flashvm_tests_color_matrix_filter_allocation" OR
       target STREQUAL "flashvm_tests_convolution_filter_allocation")
      # Exhaustive graph reconstruction failures run longer under ASan/UBSan.
      set_tests_properties(${target} PROPERTIES TIMEOUT 180)
    endif()
  endif()
endfunction()
if(FVM_BUILD_TESTS)
  file(GLOB FVM_TEST_SOURCES tests/*.c tests/*.cpp)
  foreach(source ${FVM_TEST_SOURCES})
    fvm_add_program(${source})
  endforeach()
endif()
if(FVM_BUILD_EXAMPLES)
  file(GLOB_RECURSE FVM_EXAMPLE_SOURCES examples/*/main.c examples/*/main.cpp)
  foreach(source ${FVM_EXAMPLE_SOURCES})
    fvm_add_program(${source})
  endforeach()
endif()

# Separate processes verify that persisted graph/callback references do not
# depend on an originating process address. Fixture ordering also supports -j.
if(FVM_BUILD_TESTS)
  set_tests_properties(flashvm_tests_file_process_1_writer PROPERTIES
    FIXTURES_SETUP file_process_state)
  set_tests_properties(flashvm_tests_file_process_2_reader PROPERTIES
    FIXTURES_REQUIRED file_process_state)
endif()

if(FVM_BUILD_TESTS)
  find_package(Python3 COMPONENTS Interpreter QUIET)
endif()
if(FVM_BUILD_TESTS AND FVM_ENABLE_OS_PORTS)
  if(Python3_Interpreter_FOUND)
    add_test(NAME flashvm_network_independent_processes
      COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_resource_process.py
              $<TARGET_FILE:flashvm_tests_resource_network_process>)
    set_tests_properties(flashvm_network_independent_processes PROPERTIES TIMEOUT 35)
  endif()
endif()

if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND)
  add_test(NAME flashvm_error_independent_processes
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_error_process.py
            $<TARGET_FILE:flashvm_tests_error_transfer_process>)
  set_tests_properties(flashvm_error_independent_processes PROPERTIES TIMEOUT 60)
endif()

if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND)
  add_test(NAME flashvm_objects_independent_processes
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_objects_process.py
            $<TARGET_FILE:flashvm_tests_objects_transfer_process>)
  set_tests_properties(flashvm_objects_independent_processes PROPERTIES TIMEOUT 90)
  if(FVM_ENABLE_OS_PORTS)
    add_test(NAME flashvm_objects_network_processes
      COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_objects_network.py
              $<TARGET_FILE:flashvm_tests_objects_transfer_process>)
    set_tests_properties(flashvm_objects_network_processes PROPERTIES TIMEOUT 60)
  endif()
endif()

# POSIX TZ rule strings are a libc calendar-port check, not cross-platform proof.
if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND UNIX)
  add_test(NAME flashvm_date_timezone_profiles
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_date_timezones.py
            $<TARGET_FILE:flashvm_tests_date_timezone>)
  set_tests_properties(flashvm_date_timezone_profiles PROPERTIES TIMEOUT 60)
endif()

# Optional corpus runner; its smoke test generates bounded fixtures without external downloads.
if(FVM_BUILD_REFERENCE_RUNNER)
  add_executable(flashvm_reference_swf tools/reference_swf.c)
  target_link_libraries(flashvm_reference_swf PRIVATE flashvm)
  if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND)
    add_test(NAME flashvm_reference_runner
      COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_reference_runner.py
              $<TARGET_FILE:flashvm_reference_swf>)
    set_tests_properties(flashvm_reference_runner PROPERTIES TIMEOUT 60)
  endif()
endif()
