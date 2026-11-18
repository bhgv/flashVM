# Copyright (c) 2026 bhgv
# EN: Register VM test/example executables with explicit SDK exclusions.
# RU: Регистрация VM-тестов и примеров с явными исключениями для SDK.

# EN: Register one source as a linked program, excluding unavailable SDK profiles.
# RU: Зарегистрировать source как программу, исключая недоступные SDK-профили.
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
    if(relative STREQUAL "examples/embedding/native_module/main.c")
        target_sources(${target} PRIVATE "${PROJECT_SOURCE_DIR}/examples/embedding/native_module/module.c")
        set_property(TARGET ${target} PROPERTY C_STANDARD 90)
    endif()
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
