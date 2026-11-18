# Copyright (c) 2026 bhgv
# EN: Process, platform and optional media acceptance tests.
# RU: Тесты процессов, платформ и опционального multimedia.

# Separate processes verify that persisted graph/callback references do not
# depend on an originating process address. Fixture ordering also supports -j.
if(FVM_BUILD_TESTS)
    set_tests_properties(flashvm_tests_file_process_1_writer PROPERTIES
        FIXTURES_SETUP file_process_state)
    set_tests_properties(flashvm_tests_file_process_2_reader PROPERTIES
        FIXTURES_REQUIRED file_process_state)
endif()

if(FVM_BUILD_TESTS)
    # FindPython3 starts at CMake 3.12; retain standalone 3.10 builds.
    if(CMAKE_VERSION VERSION_LESS "3.12")
        find_package(PythonInterp 3 QUIET)
        set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
        set(Python3_Interpreter_FOUND "${PYTHONINTERP_FOUND}")
    else()
        find_package(Python3 COMPONENTS Interpreter QUIET)
    endif()
endif()
if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING AND TARGET flashvm_examples_embedding_gradients_main)
    add_test(NAME flashvm_gradient_strokes_tutorial
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_gradient_strokes_example.py
                        $<TARGET_FILE:flashvm_examples_embedding_gradients_main>)
    set_tests_properties(flashvm_gradient_strokes_tutorial PROPERTIES TIMEOUT 20 SKIP_RETURN_CODE 77)
    add_test(NAME flashvm_gradient_linear_rgb_tutorial
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_gradient_linear_rgb_example.py
                        $<TARGET_FILE:flashvm_examples_embedding_gradients_main>)
    set_tests_properties(flashvm_gradient_linear_rgb_tutorial PROPERTIES TIMEOUT 20 SKIP_RETURN_CODE 77)
    add_test(NAME flashvm_gradient_focal_tutorial
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_gradient_focal_example.py
                        $<TARGET_FILE:flashvm_examples_embedding_gradients_main>)
    set_tests_properties(flashvm_gradient_focal_tutorial PROPERTIES TIMEOUT 20 SKIP_RETURN_CODE 77)
endif()
if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING AND TARGET flashvm_examples_embedding_text_layout_main)
    add_test(NAME flashvm_text_kerning_tutorial
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_text_kerning_example.py
                        $<TARGET_FILE:flashvm_examples_embedding_text_layout_main>)
    set_tests_properties(flashvm_text_kerning_tutorial PROPERTIES TIMEOUT 20 SKIP_RETURN_CODE 77)
endif()
if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING AND TARGET flashvm_examples_embedding_standard_text_main)
    add_test(NAME flashvm_text_autosize_tutorial
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_text_autosize_example.py
                        $<TARGET_FILE:flashvm_examples_embedding_standard_text_main>)
    set_tests_properties(flashvm_text_autosize_tutorial PROPERTIES TIMEOUT 20 SKIP_RETURN_CODE 77)
    add_test(NAME flashvm_text_scroll_tutorial
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_text_scroll_example.py
                        $<TARGET_FILE:flashvm_examples_embedding_standard_text_main>)
    set_tests_properties(flashvm_text_scroll_tutorial PROPERTIES TIMEOUT 20 SKIP_RETURN_CODE 77)
    add_test(NAME flashvm_text_scroll_events_tutorial
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_text_scroll_events_example.py
                        $<TARGET_FILE:flashvm_examples_embedding_standard_text_main>)
    set_tests_properties(flashvm_text_scroll_events_tutorial PROPERTIES TIMEOUT 20 SKIP_RETURN_CODE 77)
endif()
if(FVM_BUILD_TESTS AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME flashvm_windows_pe_inspector
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_windows_pe_inspector.py)
    set_tests_properties(flashvm_windows_pe_inspector PROPERTIES TIMEOUT 10)
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
        add_test(NAME flashvm_swf_free_mp3_export
            COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_free_mp3_example.py
                            $<TARGET_FILE:flashvm_examples_multimedia_swf_stream_mp3_main>)
        set_tests_properties(flashvm_swf_free_mp3_export PROPERTIES TIMEOUT 30)
        add_test(NAME flashvm_swf_crc_mp3_export
            COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_free_mp3_example.py
                            $<TARGET_FILE:flashvm_examples_multimedia_swf_stream_mp3_main> --crc)
        set_tests_properties(flashvm_swf_crc_mp3_export PROPERTIES TIMEOUT 30)
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
    if(Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
        add_test(NAME flashvm_swf_stream_mp3_seek_tutorial
            COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_mp3_seek_example.py
                $<TARGET_FILE:flashvm_examples_multimedia_swf_stream_mp3_main>)
        set_tests_properties(flashvm_swf_stream_mp3_seek_tutorial PROPERTIES
            TIMEOUT 120 SKIP_RETURN_CODE 77)
    endif()
endif()

# The VM remains independently embeddable; a desktop SDL2 SDK is needed only on explicit request.
option(FVM_BUILD_DESKTOP_FRONTEND "Build the SDL2 desktop SWF player" OFF)
if(FVM_BUILD_DESKTOP_FRONTEND)
    if(NOT FVM_ENABLE_AGG AND NOT FVM_ENABLE_CAIRO)
        message(FATAL_ERROR "Desktop playback requires GRAPHICS_AGG or GRAPHICS_CAIRO")
    endif()
    add_subdirectory(frontend)
endif()

# Check the bounded C89 MP3 exporter using native voices and normalized streams after VM teardown.
if(FVM_BUILD_TESTS AND FVM_BUILD_EXAMPLES AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME flashvm_swf_queued_identity_tutorial
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_swf_queued_identity_example.py
                        $<TARGET_FILE:flashvm_examples_multimedia_swf_queued_identity_main>)
    set_tests_properties(flashvm_swf_queued_identity_tutorial PROPERTIES TIMEOUT 15)
    add_test(NAME flashvm_swf_voice_reuse_tutorial
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_swf_voice_reuse_example.py
                        $<TARGET_FILE:flashvm_examples_multimedia_swf_voice_reuse_main>)
    set_tests_properties(flashvm_swf_voice_reuse_tutorial PROPERTIES TIMEOUT 30)
    add_test(NAME flashvm_swf_mp3_tutorial
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_swf_mp3_example.py
            $<TARGET_FILE:flashvm_examples_multimedia_swf_mp3_main>)
    set_tests_properties(flashvm_swf_mp3_tutorial PROPERTIES TIMEOUT 30)
endif()

# Check literal partial-ADPCM C89 export in both prepared and native encoded-range modes.
if(FVM_BUILD_TESTS AND FVM_BUILD_EXAMPLES AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME flashvm_swf_stream_adpcm_tutorial
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_swf_stream_adpcm_example.py
            $<TARGET_FILE:flashvm_examples_multimedia_swf_stream_adpcm_main>)
    set_tests_properties(flashvm_swf_stream_adpcm_tutorial PROPERTIES TIMEOUT 30)
endif()

# Exercise the detached stream/event C89 export with an independent rational sample oracle.
if(FVM_BUILD_TESTS AND FVM_BUILD_EXAMPLES AND Python3_Interpreter_FOUND AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME flashvm_swf_stream_mix_tutorial
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/check_swf_stream_mix_example.py
            $<TARGET_FILE:flashvm_examples_multimedia_swf_stream_mix_main>)
    set_tests_properties(flashvm_swf_stream_mix_tutorial PROPERTIES TIMEOUT 30)
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
