# Copyright (c) 2026 bhgv
# EN: Exercise the documented bidirectional RPC scenarios with actual DisVM processes.
# RU: Проверка документированных двусторонних RPC-сценариев в настоящих процессах DisVM.
set(disvm_examples "${CMAKE_CURRENT_SOURCE_DIR}/examples/disvm")
set(disvm_runtime "${STYXRPC_DISVM_BUILD}/disvm")
set(disvm_limbo "${STYXRPC_DISVM_BUILD}/bootstrap/limbo-bootstrap")
set(disvm_upstream_modules "${STYXRPC_DISVM_SOURCE}/upstream/module")
set(disvm_native_modules "${STYXRPC_DISVM_SOURCE}/module")
foreach(required_path IN ITEMS "${disvm_runtime}" "${disvm_limbo}"
        "${disvm_upstream_modules}/sys.m" "${disvm_native_modules}/vfsnative.m")
    if(NOT EXISTS "${required_path}")
        message(FATAL_ERROR "Missing DisVM integration test input: ${required_path}")
    endif()
endforeach()
set(disvm_command "${Python3_EXECUTABLE}")
set(disvm_common --runtime "${disvm_runtime}" --limbo "${disvm_limbo}")

foreach(mode IN ITEMS interpreter jit)
    set(mode_args)
    if(mode STREQUAL "jit")
        list(APPEND mode_args --jit)
    endif()
    add_test(NAME styxrpc-disvm-load-${mode}
        COMMAND "${disvm_command}" "${disvm_examples}/run.py" ${disvm_common}
            --module "${disvm_upstream_modules}" --device "$<TARGET_FILE:styxrpc-device>"
            ${mode_args})
    add_test(NAME styxrpc-disvm-channels-${mode}
        COMMAND "${disvm_command}" "${disvm_examples}/run.py" ${disvm_common}
            --module "${disvm_upstream_modules}" --device "$<TARGET_FILE:styxrpc-channel-device>"
            --channels ${mode_args})
endforeach()
add_test(NAME styxrpc-disvm-launch
    COMMAND "${disvm_command}" "${disvm_examples}/launch.py" ${disvm_common}
        --module "${disvm_native_modules}" --driver "$<TARGET_FILE:styxrpc-launch>")
add_test(NAME styxrpc-disvm-named
    COMMAND "${disvm_command}" "${disvm_examples}/named.py" ${disvm_common}
        --module "${disvm_upstream_modules}" --native-module "${disvm_native_modules}"
        --driver "$<TARGET_FILE:styxrpc-named>")
add_test(NAME styxrpc-disvm-routed
    COMMAND "${disvm_command}" "${disvm_examples}/routed.py" ${disvm_common}
        --disvm-source "${STYXRPC_DISVM_SOURCE}" --driver "$<TARGET_FILE:styxrpc-routed-client>")

if(STYXRPC_FLASHVM_TEST_HOST)
    if(NOT EXISTS "${STYXRPC_FLASHVM_TEST_HOST}")
        message(FATAL_ERROR "Missing FlashVM bridge test host: ${STYXRPC_FLASHVM_TEST_HOST}")
    endif()
    foreach(direction IN ITEMS flash-to-dis dis-to-flash)
        foreach(mode IN ITEMS interpreter jit)
            set(mode_args)
            if(mode STREQUAL "jit")
                list(APPEND mode_args --jit)
            endif()
            foreach(profile IN ITEMS scalar managed ref graph hybrid channel channel-args channel-incoming channel-delegated channel-pid-graph channel-pid-result object-image module-load playback playback-movies)
                if(((profile STREQUAL "channel-args" OR profile STREQUAL "channel-delegated" OR profile STREQUAL "channel-pid-graph" OR profile STREQUAL "module-load")
                        AND NOT direction STREQUAL "flash-to-dis") OR
                   ((profile STREQUAL "channel-incoming" OR profile STREQUAL "channel-pid-result" OR profile STREQUAL "object-image" OR profile STREQUAL "playback" OR profile STREQUAL "playback-movies")
                        AND NOT direction STREQUAL "dis-to-flash"))
                    continue()
                endif()
                set(profile_args)
                set(prefix "")
                if(profile STREQUAL "ref")
                    set(prefix "ref-")
                    set(profile_args --refs)
                elseif(NOT profile STREQUAL "scalar")
                    set(prefix "${profile}-")
                    set(profile_args "--${profile}")
                endif()
                set(test_name "styxrpc-${prefix}${direction}-${mode}")
                add_test(NAME "${test_name}"
                    COMMAND "${disvm_command}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/flashvm/run.py"
                        ${disvm_common} --source "${STYXRPC_DISVM_SOURCE}"
                        --flash-host "${STYXRPC_FLASHVM_TEST_HOST}" --direction "${direction}" ${profile_args} ${mode_args})
                set_tests_properties("${test_name}" PROPERTIES TIMEOUT 75)
            endforeach()
        endforeach()
    endforeach()
    foreach(mode IN ITEMS interpreter jit)
        set(mode_args)
        if(mode STREQUAL "jit")
            list(APPEND mode_args --jit)
        endif()
        add_test(NAME "styxrpc-flashvm-launch-${mode}"
            COMMAND "${disvm_command}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/flashvm/run.py"
                ${disvm_common} --source "${STYXRPC_DISVM_SOURCE}"
                --flash-host "${STYXRPC_FLASHVM_TEST_HOST}" --direction launch ${mode_args})
        set_tests_properties("styxrpc-flashvm-launch-${mode}" PROPERTIES TIMEOUT 75)
    endforeach()
    add_test(NAME styxrpc-flashvm-vfs
        COMMAND "${disvm_command}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/flashvm/run.py"
            ${disvm_common} --source "${STYXRPC_DISVM_SOURCE}"
            --flash-host "${STYXRPC_FLASHVM_TEST_HOST}" --direction vfs --device "$<TARGET_FILE:styxrpc-device>")
    set_tests_properties(styxrpc-flashvm-vfs PROPERTIES TIMEOUT 45)
endif()


foreach(presentation IN ITEMS player window)
    if(presentation STREQUAL "window")
        set(paired_player "${STYXRPC_FLASHVM_WINDOW_PLAYER}")
    else()
        set(paired_player "${STYXRPC_FLASHVM_PLAYER}")
    endif()
    if(NOT paired_player)
        continue()
    endif()
    if(NOT EXISTS "${paired_player}")
        message(FATAL_ERROR "Missing paired FlashVM ${presentation}: ${paired_player}")
    endif()
    foreach(mode IN ITEMS interpreter jit)
        set(mode_args)
        if(mode STREQUAL "jit")
            list(APPEND mode_args --jit)
        endif()
        set(test_name "styxrpc-native-${presentation}-${mode}")
        add_test(NAME "${test_name}"
            COMMAND "${disvm_command}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/flashvm/run.py"
                ${disvm_common} --source "${STYXRPC_DISVM_SOURCE}" --flash-host "${paired_player}"
                --direction "native-${presentation}" ${mode_args})
        set_tests_properties("${test_name}" PROPERTIES TIMEOUT 45)
        if(presentation STREQUAL "window")
            set_tests_properties("${test_name}" PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy")
        endif()
    endforeach()
endforeach()
