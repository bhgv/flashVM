cmake_minimum_required(VERSION 3.10)
include(cmake/ProjectVersion.cmake)
project(FlashVM VERSION "${FVM_VERSION}" LANGUAGES C CXX)

# The VM and bundled backends must build without modern C/C++ language modes.
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(FVM_PLAYER_HOST_SHARED "Build the optional player host as a shared module" OFF)
if(ANDROID OR FVM_PLAYER_HOST_SHARED)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
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
                                 src/avm1/*.cpp src/swf/*.cpp src/standard/*.cpp src/jit/*.cpp
                                 src/multimedia/*.cpp)
list(REMOVE_ITEM FVM_CORE_SOURCES ${PROJECT_SOURCE_DIR}/src/capi/sqlite_c.cpp)
add_library(flashvm_srecord STATIC src/protocol/srecord.c src/protocol/sbindings.c src/protocol/sstream.c src/protocol/sslot.c src/protocol/scontrol.c src/protocol/smedia.c src/protocol/sadapt.c src/protocol/rdispatch.c src/protocol/rwire.c src/protocol/rheartbeat.c src/protocol/rfs.c src/protocol/r9p.c)
target_include_directories(flashvm_srecord PUBLIC ${PROJECT_SOURCE_DIR}/include)
add_library(flashvm STATIC src/protocol/srecord.c src/protocol/sbindings.c src/protocol/sstream.c src/protocol/sslot.c src/protocol/scontrol.c src/protocol/smedia.c src/protocol/sadapt.c src/protocol/rdispatch.c src/protocol/rwire.c src/protocol/rheartbeat.c src/protocol/rfs.c src/protocol/r9p.c ${FVM_CORE_SOURCES} src/render/raster_c.cpp
  ports/code_port.c ports/mac/code_arm64.cpp ports/file_port.c ports/calendar_port.c ports/${FVM_PORT}/clock.c ports/${FVM_PORT}/monotonic.c ports/${FVM_PORT}/thread.c ports/${FVM_PORT}/channel_sync.c
  external/mini_regex/mini_regex.c)
target_include_directories(flashvm PUBLIC ${PROJECT_SOURCE_DIR}/include ${FVM_GENERATED_INCLUDE}
  PRIVATE ${PROJECT_SOURCE_DIR}/ports ${PROJECT_SOURCE_DIR}/external/mini_regex)
target_compile_definitions(flashvm PUBLIC ${FVM_BUILD_KEYS})
option(FVM_ENABLE_MACOS_ARM64_JIT "Opt in to macOS 11.4+ ARM64 MAP_JIT callback publication" OFF)
if(FVM_ENABLE_MACOS_ARM64_JIT)
  if(NOT APPLE OR NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(FATAL_ERROR "FVM_ENABLE_MACOS_ARM64_JIT requires a desktop macOS toolchain")
  endif()
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET OR CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "11.4")
    message(FATAL_ERROR "macOS ARM64 JIT callback publication requires deployment target 11.4+")
  endif()
  target_compile_definitions(flashvm PUBLIC FVM_ENABLE_MACOS_ARM64_JIT=1)
endif()

# Optional host SDK: default core builds retain explicit UNSUPPORTED SDL audio stubs.
option(FVM_ENABLE_SDL_AUDIO "Enable the real SDL2 copied-PCM output queue bridge" OFF)
if(FVM_ENABLE_SDL_AUDIO)
  find_package(SDL2 2.0.9 CONFIG REQUIRED)
  target_compile_definitions(flashvm PUBLIC FVM_HAS_SDL_AUDIO=1)
  target_link_libraries(flashvm PUBLIC SDL2::SDL2)
endif()

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

# Native multimedia input wrappers are available independently of FFmpeg/OS_PORTS.
if(WIN32)
  target_link_libraries(flashvm PUBLIC user32 ole32 sapi uuid)
elseif(ANDROID)
  target_link_libraries(flashvm PUBLIC android)
elseif(APPLE)
  find_library(FVM_MULTIMEDIA_APPLICATION_SERVICES ApplicationServices)
  if(NOT FVM_MULTIMEDIA_APPLICATION_SERVICES)
    message(FATAL_ERROR "Multimedia input requires ApplicationServices on macOS")
  endif()
  target_link_libraries(flashvm PUBLIC ${FVM_MULTIMEDIA_APPLICATION_SERVICES})
endif()
if(APPLE)
  option(FVM_ENABLE_MACOS_STT "Build macOS 10.15+ Speech/AVFoundation recognition adapter" ON)
  if(FVM_ENABLE_MACOS_STT)
    if(CMAKE_VERSION VERSION_LESS 3.16)
      message(FATAL_ERROR "macOS STT requires CMake 3.16+ Objective-C++ support; or disable FVM_ENABLE_MACOS_STT")
    endif()
    if(CMAKE_OSX_DEPLOYMENT_TARGET AND CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS 10.15)
      message(FATAL_ERROR "macOS STT requires deployment target 10.15+; or disable FVM_ENABLE_MACOS_STT")
    endif()
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 98)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    set(CMAKE_OBJCXX_EXTENSIONS OFF)
    find_library(FVM_SPEECH_FRAMEWORK Speech)
    find_library(FVM_AVFOUNDATION_FRAMEWORK AVFoundation)
    find_library(FVM_FOUNDATION_FRAMEWORK Foundation)
    if(NOT FVM_SPEECH_FRAMEWORK OR NOT FVM_AVFOUNDATION_FRAMEWORK OR NOT FVM_FOUNDATION_FRAMEWORK)
      message(FATAL_ERROR "macOS STT requires Speech, AVFoundation and Foundation (10.15+ SDK); or disable FVM_ENABLE_MACOS_STT")
    endif()
    set(FVM_MACOS_STT_SOURCES src/multimedia/speech/stt/macos_service.mm
      src/multimedia/speech/stt/macos_open.mm src/multimedia/speech/stt/macos_poll.mm
      src/multimedia/speech/stt/macos_close.mm)
    # Use the actual Objective-C++ frontend while retaining C++98 for the C++ portion.
    set_source_files_properties(${FVM_MACOS_STT_SOURCES} PROPERTIES LANGUAGE OBJCXX
      COMPILE_FLAGS "-fblocks -fno-objc-arc")
    target_sources(flashvm PRIVATE ${FVM_MACOS_STT_SOURCES})
    target_compile_definitions(flashvm PRIVATE FVM_HAS_MACOS_STT=1)
    target_link_libraries(flashvm PUBLIC ${FVM_SPEECH_FRAMEWORK}
      ${FVM_AVFOUNDATION_FRAMEWORK} ${FVM_FOUNDATION_FRAMEWORK})
  endif()
endif()
option(FVM_ENABLE_LINUX_AUDIO "Build real Linux PCM output with a supplied static ALSA SDK" OFF)
if(FVM_ENABLE_LINUX_AUDIO)
  add_subdirectory(external/audio)
  target_link_libraries(flashvm PUBLIC fvm_linux_audio)
  target_compile_definitions(flashvm PRIVATE FVM_HAS_LINUX_ALSA_AUDIO=1)
endif()
option(FVM_ENABLE_LINUX_STT "Build real Linux ALSA/Vosk speech recognition with supplied static SDKs" OFF)
if(FVM_ENABLE_LINUX_STT)
  add_subdirectory(external/speech)
  target_include_directories(flashvm SYSTEM PRIVATE "${FVM_VOSK_ROOT}/include" "${FVM_ALSA_ROOT}/include")
  target_compile_definitions(flashvm PRIVATE FVM_HAS_LINUX_STT=1)
  target_link_libraries(flashvm PUBLIC fvm_linux_stt)
endif()
if(FVM_ENABLE_MEDIA)
  add_subdirectory(external/ffmpeg)
  target_include_directories(flashvm SYSTEM PRIVATE "${FVM_FFMPEG_ROOT}/include")
  target_link_libraries(flashvm PUBLIC fvm_ffmpeg)
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
# LLP64 Windows has 32-bit long: select zlib's supported 32-bit CRC braid in strict C89.
if(WIN32)
  target_compile_definitions(fvm_zlib PRIVATE Z_TESTW=4)
  # Modern MinGW limits.h exposes ULLONG_MAX even in C90 mode. Its expansion
  # inside unmodified upstream zutil.h triggers Clang's pedantic long-long
  # diagnostic. Keep this compatibility exception on zlib alone: first-party
  # C89/C++98 targets retain all strict language checks.
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fvm_zlib PRIVATE -Wno-long-long)
  endif()
endif()
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
  if(relative STREQUAL "examples/embedding/media_parts/main.c" OR relative STREQUAL "examples/embedding/srecords/main.c" OR relative STREQUAL "examples/embedding/resource_events/main.c" OR relative STREQUAL "examples/embedding/hybrid_heartbeat/main.c" OR relative STREQUAL "examples/embedding/resource_files/main.c" OR relative STREQUAL "examples/embedding/resource_9p/main.c")
    return() # The protocol tutorial deliberately links only the standalone C codec.
  endif()
  if(relative STREQUAL "tests/windows_sapi_mask.cpp" AND NOT WIN32)
    return() # This contract requires actual target Windows SDK declarations.
  endif()
  if(relative MATCHES "^examples/player/(frame_parts|frame_capture|playback_mailbox|playback_control|image_loading|unload_loading|redirect_loading|monitored_loading|async_loading|background_loading|configured_host|embedded_movie|loading|socket_loading)/main.c$")
    return() # Requires the optional player's generated metadata and SDL SDK.
  endif()
  if(relative MATCHES "^examples/frontends/(sdl|clutter3d|gtk3|qt5|linux_fb)/")
    return() # SDK-specific tutorial targets are registered by examples/frontends/CMakeLists.txt.
  endif()
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
  if(target STREQUAL "flashvm_tests_jit_macos_arm64_native" AND
     CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target} PRIVATE -fno-omit-frame-pointer)
  endif()
  if(FVM_BUILD_TESTS)
    if(target STREQUAL "flashvm_examples_player_muted_stream_main")
      add_test(NAME ${target} COMMAND ${target}
        ${PROJECT_SOURCE_DIR}/examples/player/muted_stream/muted-stream.swf)
    else()
      add_test(NAME ${target} COMMAND ${target})
    endif()
    set_tests_properties(${target} PROPERTIES TIMEOUT 30)
    if(target STREQUAL "flashvm_examples_frontends_headless_main" OR
       target STREQUAL "flashvm_tests_jit_macos_native" OR
       target STREQUAL "flashvm_tests_jit_windows_unwind" OR
       target STREQUAL "flashvm_tests_jit_macos_arm64_port" OR
       target STREQUAL "flashvm_tests_jit_macos_arm64_native" OR
       target STREQUAL "flashvm_tests_jit_native_instances")
      # Unavailable native platforms or the headless example raster backend skip explicitly.
      set_tests_properties(${target} PROPERTIES SKIP_RETURN_CODE 77)
    endif()
    if(target STREQUAL "flashvm_tests_avm1_clone_sprite_allocation" OR
       target STREQUAL "flashvm_tests_color_matrix_filter_allocation" OR
       target STREQUAL "flashvm_tests_convolution_filter_allocation" OR
       target STREQUAL "flashvm_tests_property_version_exhaustive" OR
       target STREQUAL "flashvm_tests_property_version_exhaustive_execution")
      # Exhaustive graph/attribute/allocation matrices run longer under ASan/UBSan.
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

if(FVM_BUILD_EXAMPLES)
  add_subdirectory(examples/frontends)
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
if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
  add_test(NAME flashvm_crash_archive_host
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_crash_archive.py
            $<TARGET_FILE:flashvm_tests_crash_diagnostics>
            $<TARGET_FILE:flashvm_tests_swd_sources>)
  set_tests_properties(flashvm_crash_archive_host PROPERTIES TIMEOUT 70)
  add_test(NAME flashvm_debugger_tutorial_contract
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_debugger_examples.py)
endif()
if(FVM_BUILD_TESTS AND FVM_BUILD_EXAMPLES AND FVM_ENABLE_OS_PORTS AND NOT CMAKE_CROSSCOMPILING)
  add_test(NAME flashvm_function_independent_threads
    COMMAND $<TARGET_FILE:flashvm_examples_embedding_function_rpc_main> threads)
  set_tests_properties(flashvm_function_independent_threads PROPERTIES TIMEOUT 30)
endif()
if(FVM_BUILD_TESTS AND FVM_ENABLE_OS_PORTS AND NOT CMAKE_CROSSCOMPILING)
  if(Python3_Interpreter_FOUND)
    add_test(NAME flashvm_network_independent_processes
      COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_resource_process.py
              $<TARGET_FILE:flashvm_tests_resource_network_process>)
    set_tests_properties(flashvm_network_independent_processes PROPERTIES TIMEOUT 35)
    if(FVM_BUILD_EXAMPLES)
      add_test(NAME flashvm_function_independent_processes
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_function_process.py
                $<TARGET_FILE:flashvm_examples_embedding_function_rpc_main>)
      set_tests_properties(flashvm_function_independent_processes PROPERTIES TIMEOUT 40)
    endif()
  endif()
endif()

if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
  # Use the production version module and the same native generator in an isolated fixture.
  set(FVM_VERSION_CHECK_ARGS --cmake "${CMAKE_COMMAND}"
    --module "${PROJECT_SOURCE_DIR}/cmake/ProjectVersion.cmake"
    --generator "${CMAKE_GENERATOR}")
  if(CMAKE_MAKE_PROGRAM)
    list(APPEND FVM_VERSION_CHECK_ARGS --make-program "${CMAKE_MAKE_PROGRAM}")
  endif()
  if(CMAKE_GENERATOR_PLATFORM)
    list(APPEND FVM_VERSION_CHECK_ARGS --platform "${CMAKE_GENERATOR_PLATFORM}")
  endif()
  if(CMAKE_GENERATOR_TOOLSET)
    list(APPEND FVM_VERSION_CHECK_ARGS --toolset "${CMAKE_GENERATOR_TOOLSET}")
  endif()
  add_test(NAME flashvm_build_version_refresh
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_build_version.py
            ${FVM_VERSION_CHECK_ARGS})
  set_tests_properties(flashvm_build_version_refresh PROPERTIES TIMEOUT 60)

  add_test(NAME flashvm_elf_dependencies_host
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_elf_dependencies.py)
  add_test(NAME flashvm_platform_source_host
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_platform_source.py)
  add_test(NAME flashvm_ctest_evidence_host
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_ctest_evidence.py
            --ctest ${CMAKE_CTEST_COMMAND})
  add_test(NAME flashvm_platform_native_integration_host
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_platform_native_integration.py
            --cmake ${CMAKE_COMMAND} --ctest ${CMAKE_CTEST_COMMAND} --generator "${CMAKE_GENERATOR}")
  set_tests_properties(flashvm_platform_native_integration_host PROPERTIES TIMEOUT 180)
  add_test(NAME flashvm_release_bundle_host
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_release_bundle.py)
  add_test(NAME flashvm_release_matrix_host
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_release_matrix.py)
  add_test(NAME flashvm_native_target_host
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_native_target.py)
  add_test(NAME flashvm_platform_encoder_host
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_platform_encoder.py)
  add_test(NAME flashvm_android_adapter_host
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_android_adapter.py)
  add_test(NAME flashvm_error_independent_processes
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_error_process.py
            $<TARGET_FILE:flashvm_tests_error_transfer_process>)
  set_tests_properties(flashvm_error_independent_processes PROPERTIES TIMEOUT 60)
endif()

if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
  add_test(NAME flashvm_broadcaster_independent_processes
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_broadcaster_process.py
            $<TARGET_FILE:flashvm_tests_broadcaster_transport>)
  set_tests_properties(flashvm_broadcaster_independent_processes PROPERTIES TIMEOUT 90)
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
if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND UNIX AND NOT CMAKE_CROSSCOMPILING)
  add_test(NAME flashvm_date_timezone_profiles
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_date_timezones.py
            $<TARGET_FILE:flashvm_tests_date_timezone>)
  set_tests_properties(flashvm_date_timezone_profiles PROPERTIES TIMEOUT 60)
endif()

# Optional corpus runner; its smoke test generates bounded fixtures without external downloads.
if(FVM_BUILD_REFERENCE_RUNNER)
  add_executable(flashvm_reference_swf tools/reference_swf.c)
  target_link_libraries(flashvm_reference_swf PRIVATE flashvm)
  if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME flashvm_reference_runner
      COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_reference_runner.py
              $<TARGET_FILE:flashvm_reference_swf>)
    set_tests_properties(flashvm_reference_runner PROPERTIES TIMEOUT 60)
  endif()
endif()

# Host Python process harnesses cannot launch target binaries through CTest's
# emulator property. Omit them explicitly rather than report host execution as
# target evidence. Native test executables still use CMAKE_CROSSCOMPILING_EMULATOR.
if(FVM_BUILD_TESTS AND CMAKE_CROSSCOMPILING)
  message(STATUS "Cross tests: host Python process/timezone/reference harnesses omitted")
  if(NOT CMAKE_CROSSCOMPILING_EMULATOR)
    message(STATUS "Cross tests are build-only until a target execution adapter is configured")
  endif()
endif()

if(FVM_BUILD_TESTS AND FVM_BUILD_EXAMPLES AND Python3_Interpreter_FOUND AND
   CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING)
  add_test(NAME flashvm_speech_ssip_protocol_host
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_speech_ssip.py
      $<TARGET_FILE:flashvm_examples_multimedia_speech_tts_main>)
  set_tests_properties(flashvm_speech_ssip_protocol_host PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
endif()

if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
  if(FVM_BUILD_EXAMPLES)
    add_test(NAME flashvm_jit_instances_native_tutorial_host
      COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_jit_instances_example.py
              $<TARGET_FILE:flashvm_examples_embedding_jit_instances_main>)
    set_tests_properties(flashvm_jit_instances_native_tutorial_host PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
    add_test(NAME flashvm_swf_stream_tutorial_host
      COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_swf_stream_example.py
              $<TARGET_FILE:flashvm_examples_multimedia_swf_stream_main>)
    set_tests_properties(flashvm_swf_stream_tutorial_host PROPERTIES TIMEOUT 30)
  endif()
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_test(NAME flashvm_macos_jit_policy
      COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_macos_jit_policy.py
              --cc ${CMAKE_C_COMPILER})
  endif()
  add_test(NAME flashvm_frontend_tutorial_contracts
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_frontend_examples.py)
  add_test(NAME flashvm_documentation_source_link_order
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/test_documentation_source_links.py)
  add_test(NAME flashvm_multimedia_tutorial_contract
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_multimedia_examples.py)
endif()

if(TARGET flashvm_tests_speech_unix_connect AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
   AND NOT ANDROID AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # Interpose only endpoint creation/connect in this fixture; transport uses real socketpair I/O.
  set_property(TARGET flashvm_tests_speech_unix_connect APPEND_STRING PROPERTY
    LINK_FLAGS " -Wl,--wrap=socket -Wl,--wrap=connect")
endif()

# The native output fixture uses ALSA's real built-in null PCM, not physical hardware.
if(FVM_ENABLE_LINUX_AUDIO AND TARGET flashvm_tests_audio_alsa)
  target_compile_definitions(flashvm_tests_audio_alsa PRIVATE FVM_TEST_LINUX_AUDIO=1)
  set_tests_properties(flashvm_tests_audio_alsa PROPERTIES
    ENVIRONMENT "ALSA_CONFIG_PATH=${PROJECT_SOURCE_DIR}/tests/alsa_null.conf")
endif()
if(FVM_ENABLE_LINUX_AUDIO AND TARGET flashvm_tests_audio_alsa_faults)
  target_compile_definitions(flashvm_tests_audio_alsa_faults PRIVATE FVM_TEST_LINUX_AUDIO=1)
  set_property(TARGET flashvm_tests_audio_alsa_faults APPEND_STRING PROPERTY LINK_FLAGS
    " -Wl,--wrap=snd_pcm_writei -Wl,--wrap=snd_pcm_drop -Wl,--wrap=snd_pcm_prepare -Wl,--wrap=snd_pcm_start")
  set_tests_properties(flashvm_tests_audio_alsa_faults PROPERTIES
    ENVIRONMENT "ALSA_CONFIG_PATH=${PROJECT_SOURCE_DIR}/tests/alsa_null.conf")
endif()
if(FVM_ENABLE_LINUX_AUDIO AND TARGET flashvm_tests_swf_sound_alsa)
  target_compile_definitions(flashvm_tests_swf_sound_alsa PRIVATE FVM_TEST_LINUX_AUDIO=1)
  set_tests_properties(flashvm_tests_swf_sound_alsa PROPERTIES
    ENVIRONMENT "ALSA_CONFIG_PATH=${PROJECT_SOURCE_DIR}/tests/alsa_null.conf")
endif()

# Execute the detached encoded-stream tutorial, not just its side-effect-free usage path.
if(FVM_BUILD_TESTS AND FVM_BUILD_EXAMPLES)
  add_test(NAME flashvm_swf_stream_mp3_tutorial
    COMMAND flashvm_examples_multimedia_swf_stream_mp3_main --demo)
  set_tests_properties(flashvm_swf_stream_mp3_tutorial PROPERTIES
    PASS_REGULAR_EXPRESSION "frame=1 declared=576 seek=-3 bytes=3" TIMEOUT 30)
endif()

# The VM remains independently embeddable; a desktop SDL2 SDK is needed only on explicit request.
option(FVM_BUILD_DESKTOP_FRONTEND "Build the SDL2 desktop SWF player" OFF)
if(FVM_BUILD_DESKTOP_FRONTEND)
  if(NOT FVM_ENABLE_AGG AND NOT FVM_ENABLE_CAIRO)
    message(FATAL_ERROR "Desktop playback requires GRAPHICS_AGG or GRAPHICS_CAIRO")
  endif()
  add_subdirectory(frontend)
endif()

# Run the actual annotated software mixer, including optional native file I/O and bad CLI.
if(FVM_BUILD_TESTS AND FVM_BUILD_EXAMPLES AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
  add_test(NAME flashvm_swf_event_mixer_tutorial
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/swf_event_mixer_tutorial.py
      $<TARGET_FILE:flashvm_examples_multimedia_swf_event_mixer_main>)
  set_tests_properties(flashvm_swf_event_mixer_tutorial PROPERTIES TIMEOUT 30)
endif()

# This exercises actual SDL SDK queue/pause/consumption using an explicit dummy driver, not audible acceptance.
option(FVM_SDL_AUDIO_DEVICE_TESTS "Run SDL2 dummy-driver queue protocol test" OFF)
if(FVM_BUILD_TESTS AND FVM_ENABLE_SDL_AUDIO AND FVM_SDL_AUDIO_DEVICE_TESTS AND NOT CMAKE_CROSSCOMPILING)
  add_test(NAME flashvm_sdl_queue_dummy COMMAND flashvm_tests_sdl_audio_queue --dummy)
  add_test(NAME flashvm_sdl_quantum_dummy COMMAND flashvm_tests_sdl_audio_quantum --dummy)
  set_tests_properties(flashvm_sdl_quantum_dummy PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy" TIMEOUT 10)
  set_tests_properties(flashvm_sdl_queue_dummy PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy" TIMEOUT 10)
endif()

# Execute the annotated model tutorial without native device or SDK requirements.
if(FVM_BUILD_TESTS AND FVM_BUILD_EXAMPLES AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
  add_test(NAME flashvm_swf_audio_queue_tutorial
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/swf_audio_queue_tutorial.py
      $<TARGET_FILE:flashvm_examples_multimedia_swf_audio_queue_main>)
  set_tests_properties(flashvm_swf_audio_queue_tutorial PROPERTIES TIMEOUT 30)
endif()

# Standalone wire-format oracle needs no VM, SDL, devices or socket provider.
if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
  add_library(flashvm_srecord_oracle SHARED src/protocol/srecord.c src/protocol/sbindings.c src/protocol/sstream.c src/protocol/sslot.c src/protocol/scontrol.c src/protocol/smedia.c src/protocol/sadapt.c src/protocol/rdispatch.c src/protocol/rwire.c src/protocol/rheartbeat.c src/protocol/rfs.c src/protocol/r9p.c)
  target_include_directories(flashvm_srecord_oracle PRIVATE ${PROJECT_SOURCE_DIR}/include)
  set_target_properties(flashvm_srecord_oracle PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)
  add_test(NAME flashvm_srecord_wire_oracle COMMAND ${Python3_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tests/protocol/srecord_oracle.py $<TARGET_FILE:flashvm_srecord_oracle>)
  set_tests_properties(flashvm_srecord_wire_oracle PROPERTIES TIMEOUT 60)
endif()
if(TARGET flashvm_srecord_oracle)
  add_test(NAME flashvm_srecord_bindings_oracle COMMAND ${Python3_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tests/protocol/sbindings_oracle.py $<TARGET_FILE:flashvm_srecord_oracle>)
  set_tests_properties(flashvm_srecord_bindings_oracle PROPERTIES TIMEOUT 60)
endif()
if(TARGET flashvm_srecord_oracle)
  add_test(NAME flashvm_srecord_stream_oracle COMMAND ${Python3_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tests/protocol/sstream_oracle.py $<TARGET_FILE:flashvm_srecord_oracle>)
  set_tests_properties(flashvm_srecord_stream_oracle PROPERTIES TIMEOUT 60)
endif()

if(FVM_BUILD_EXAMPLES)
  add_executable(flashvm_examples_embedding_srecords_main examples/embedding/srecords/main.c)
  target_link_libraries(flashvm_examples_embedding_srecords_main PRIVATE flashvm_srecord)
  if(FVM_BUILD_TESTS)
    add_test(NAME flashvm_srecord_tutorial_memory COMMAND flashvm_examples_embedding_srecords_main)
  endif()
endif()
if(FVM_BUILD_TESTS)
  add_executable(flashvm_srecord_cpp98 tests/protocol/cpp98.cpp)
  target_link_libraries(flashvm_srecord_cpp98 PRIVATE flashvm_srecord)
  add_test(NAME flashvm_srecord_cpp98 COMMAND flashvm_srecord_cpp98)
  add_executable(flashvm_srecord_fuzz tests/protocol/srecord_fuzz.c)
  target_link_libraries(flashvm_srecord_fuzz PRIVATE flashvm_srecord)
  add_test(NAME flashvm_srecord_fuzz COMMAND flashvm_srecord_fuzz)
  set_tests_properties(flashvm_srecord_fuzz PROPERTIES TIMEOUT 60)
endif()
if(TARGET flashvm_examples_embedding_srecords_main AND FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
  add_test(NAME flashvm_srecord_tutorial_file COMMAND ${Python3_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tests/protocol/srecord_example.py $<TARGET_FILE:flashvm_examples_embedding_srecords_main>)
  set_tests_properties(flashvm_srecord_tutorial_file PROPERTIES TIMEOUT 20)
endif()

# Native dispatcher contracts do not need VM startup or presentation SDKs.
if(FVM_BUILD_TESTS)
  add_executable(flashvm_rdispatch_contract tests/resources/dispatch.c)
  target_include_directories(flashvm_rdispatch_contract PRIVATE ${PROJECT_SOURCE_DIR}/include)
  add_test(NAME flashvm_rdispatch_contract COMMAND flashvm_rdispatch_contract)
endif()
if(TARGET flashvm_srecord_oracle)
  add_test(NAME flashvm_rdispatch_wire COMMAND ${Python3_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tests/resources/wire.py $<TARGET_FILE:flashvm_srecord_oracle>)
  set_tests_properties(flashvm_rdispatch_wire PROPERTIES TIMEOUT 30)
endif()
if(FVM_BUILD_TESTS AND UNIX AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING)
  find_package(Threads REQUIRED)
  add_executable(flashvm_rdispatch_threads tests/resources/threads.c)
  target_link_libraries(flashvm_rdispatch_threads PRIVATE flashvm_srecord Threads::Threads)
  add_test(NAME flashvm_rdispatch_threads COMMAND flashvm_rdispatch_threads)
  set_tests_properties(flashvm_rdispatch_threads PROPERTIES TIMEOUT 30)
endif()

if(FVM_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
  add_executable(flashvm_revents_contract tests/resources/events.c)
  target_include_directories(flashvm_revents_contract PRIVATE ${PROJECT_SOURCE_DIR}/include)
  add_test(NAME flashvm_revents_contract COMMAND flashvm_revents_contract)
endif()
if(FVM_BUILD_TESTS AND UNIX AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING)
  add_executable(flashvm_revents_threads tests/resources/event_threads.c)
  target_link_libraries(flashvm_revents_threads PRIVATE flashvm_srecord Threads::Threads)
  add_test(NAME flashvm_revents_threads COMMAND flashvm_revents_threads)
  set_tests_properties(flashvm_revents_threads PROPERTIES TIMEOUT 30)
endif()
if(FVM_BUILD_EXAMPLES)
  add_executable(flashvm_examples_embedding_resource_events_main examples/embedding/resource_events/main.c)
  target_link_libraries(flashvm_examples_embedding_resource_events_main PRIVATE flashvm_srecord)
  if(FVM_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME flashvm_revents_tutorial COMMAND flashvm_examples_embedding_resource_events_main)
  endif()
endif()
if(FVM_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
  add_executable(flashvm_rdebug_contract tests/resources/rdebug.c)
  target_link_libraries(flashvm_rdebug_contract PRIVATE flashvm)
  add_test(NAME flashvm_rdebug_contract COMMAND flashvm_rdebug_contract)
  set_tests_properties(flashvm_rdebug_contract PROPERTIES TIMEOUT 30)
endif()
if(TARGET flashvm_srecord_oracle)
  add_test(NAME flashvm_revents_wire COMMAND ${Python3_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tests/resources/event_wire.py $<TARGET_FILE:flashvm_srecord_oracle>)
  set_tests_properties(flashvm_revents_wire PROPERTIES TIMEOUT 30)
endif()
if(FVM_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
  add_executable(flashvm_rdebug_failure tests/resources/rdebug_failure.c)
  target_link_libraries(flashvm_rdebug_failure PRIVATE flashvm)
  add_test(NAME flashvm_rdebug_failure COMMAND flashvm_rdebug_failure)
endif()
if(FVM_BUILD_TESTS AND UNIX AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING)
  add_executable(flashvm_rdebug_threads tests/resources/rdebug_threads.c)
  target_link_libraries(flashvm_rdebug_threads PRIVATE flashvm Threads::Threads)
  add_test(NAME flashvm_rdebug_threads COMMAND flashvm_rdebug_threads)
  set_tests_properties(flashvm_rdebug_threads PROPERTIES TIMEOUT 30)
endif()

if(FVM_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
  add_executable(flashvm_rheartbeat_contract tests/resources/heartbeat.c)
  target_link_libraries(flashvm_rheartbeat_contract PRIVATE flashvm_srecord)
  add_test(NAME flashvm_rheartbeat_contract COMMAND flashvm_rheartbeat_contract)
endif()
if(TARGET flashvm_srecord_oracle)
  add_test(NAME flashvm_rheartbeat_wire COMMAND ${Python3_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tests/resources/heartbeat_wire.py $<TARGET_FILE:flashvm_srecord_oracle>)
  set_tests_properties(flashvm_rheartbeat_wire PROPERTIES TIMEOUT 30)
endif()

if(FVM_BUILD_EXAMPLES)
  add_executable(flashvm_examples_embedding_hybrid_heartbeat_main examples/embedding/hybrid_heartbeat/main.c)
  target_link_libraries(flashvm_examples_embedding_hybrid_heartbeat_main PRIVATE flashvm_srecord)
  if(FVM_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME flashvm_rheartbeat_tutorial COMMAND flashvm_examples_embedding_hybrid_heartbeat_main)
  endif()
endif()

if(FVM_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
  add_executable(flashvm_rfs_contract tests/resources/filesystem.c)
  target_link_libraries(flashvm_rfs_contract PRIVATE flashvm_srecord)
  add_test(NAME flashvm_rfs_contract COMMAND flashvm_rfs_contract)
endif()

if(FVM_BUILD_EXAMPLES)
  add_executable(flashvm_examples_embedding_resource_files_main examples/embedding/resource_files/main.c)
  target_link_libraries(flashvm_examples_embedding_resource_files_main PRIVATE flashvm_srecord)
  if(FVM_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME flashvm_rfs_tutorial COMMAND flashvm_examples_embedding_resource_files_main)
  endif()
endif()

if(TARGET flashvm_srecord_oracle)
  add_test(NAME flashvm_r9p_wire COMMAND ${Python3_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tests/resources/ninep.py $<TARGET_FILE:flashvm_srecord_oracle>)
  set_tests_properties(flashvm_r9p_wire PROPERTIES TIMEOUT 30)
endif()
if(FVM_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
  add_executable(flashvm_r9p_debugger tests/resources/ninep_debugger.c)
  target_link_libraries(flashvm_r9p_debugger PRIVATE flashvm)
  add_test(NAME flashvm_r9p_debugger COMMAND flashvm_r9p_debugger)
endif()

if(FVM_BUILD_EXAMPLES)
  add_executable(flashvm_examples_embedding_resource_9p_main examples/embedding/resource_9p/main.c)
  target_link_libraries(flashvm_examples_embedding_resource_9p_main PRIVATE flashvm_srecord)
  if(FVM_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME flashvm_r9p_tutorial COMMAND flashvm_examples_embedding_resource_9p_main)
  endif()
endif()
if(FVM_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
  add_executable(flashvm_r9p_lifetime tests/resources/ninep_lifetime.c)
  target_link_libraries(flashvm_r9p_lifetime PRIVATE flashvm_srecord)
  add_test(NAME flashvm_r9p_lifetime COMMAND flashvm_r9p_lifetime)
endif()

# Fixed slot boundaries isolate damaged media records without trusting their embedded lengths.
if(TARGET flashvm_srecord_oracle)
  add_test(NAME flashvm_srecord_slot_oracle COMMAND ${Python3_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tests/protocol/sslot_oracle.py $<TARGET_FILE:flashvm_srecord_oracle>)
  set_tests_properties(flashvm_srecord_slot_oracle PROPERTIES TIMEOUT 30)
endif()

# Original file/live fragment policy is independent of VM, media codecs and socket providers.
if(TARGET flashvm_srecord_oracle)
  add_test(NAME flashvm_srecord_media_oracle COMMAND ${Python3_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tests/protocol/smedia_oracle.py $<TARGET_FILE:flashvm_srecord_oracle>)
  set_tests_properties(flashvm_srecord_media_oracle PROPERTIES TIMEOUT 30)
endif()

# Detachable C89 media tutorial links only the standalone protocol library.
if(FVM_BUILD_EXAMPLES)
  add_executable(flashvm_examples_embedding_media_parts_main examples/embedding/media_parts/main.c)
  target_link_libraries(flashvm_examples_embedding_media_parts_main PRIVATE flashvm_srecord)
  if(FVM_BUILD_TESTS)
    add_test(NAME flashvm_srecord_media_tutorial COMMAND flashvm_examples_embedding_media_parts_main)
  endif()
endif()

if(TARGET flashvm_srecord_oracle)
  add_test(NAME flashvm_srecord_control_oracle COMMAND ${Python3_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tests/protocol/scontrol_oracle.py $<TARGET_FILE:flashvm_srecord_oracle>)
  set_tests_properties(flashvm_srecord_control_oracle PROPERTIES TIMEOUT 30)
endif()

if(FVM_BUILD_TESTS)
  add_executable(flashvm_srecord_control_contract tests/protocol/scontrol_contract.c)
  target_link_libraries(flashvm_srecord_control_contract PRIVATE flashvm_srecord)
  add_test(NAME flashvm_srecord_control_contract COMMAND flashvm_srecord_control_contract)
endif()

# Link-quality policy is standalone and consumes trusted host measurements, never VM objects.
if(FVM_BUILD_TESTS)
  add_executable(flashvm_srecord_adapt_contract tests/protocol/sadapt_contract.c)
  target_link_libraries(flashvm_srecord_adapt_contract PRIVATE flashvm_srecord)
  add_test(NAME flashvm_srecord_adapt_contract COMMAND flashvm_srecord_adapt_contract)
endif()
if(TARGET flashvm_srecord_oracle)
  add_test(NAME flashvm_srecord_adapt_oracle COMMAND ${Python3_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/tests/protocol/sadapt_oracle.py $<TARGET_FILE:flashvm_srecord_oracle>)
  set_tests_properties(flashvm_srecord_adapt_oracle PROPERTIES TIMEOUT 30)
endif()
