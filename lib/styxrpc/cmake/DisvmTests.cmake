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
set(disvm_command "${PYTHON_EXECUTABLE}")
set(disvm_common --runtime "${disvm_runtime}" --limbo "${disvm_limbo}")

if(STYXRPC_FLASHVM_TOOLS)
    if(NOT STYXRPC_FLASHVM_PLAYER)
        message(FATAL_ERROR "Compiled pair tests require STYXRPC_FLASHVM_PLAYER")
    endif()
    set(tool_args)
    foreach(tool IN ITEMS "as2;as2/as2_viewc" "fir;fir/fir1" "fir-opt;fir/fir1_opt" "swl;linker/swl")
        list(GET tool 0 flag)
        list(GET tool 1 executable)
        set(path "${STYXRPC_FLASHVM_TOOLS}/${executable}")
        if(NOT EXISTS "${path}")
            message(FATAL_ERROR "Missing compiled pair test tool: ${path}")
        endif()
        list(APPEND tool_args "--${flag}" "${path}")
    endforeach()
    foreach(mode IN ITEMS interpreter jit)
        set(mode_args)
        if(mode STREQUAL "jit")
            list(APPEND mode_args --jit)
        endif()
        foreach(transport IN ITEMS unix tcp)
            set(transport_args)
            if(transport STREQUAL "tcp")
                list(APPEND transport_args --tcp)
            endif()
            foreach(profile IN ITEMS int-ref ref-port)
                set(test_name "styxrpc-native-${profile}-${mode}-${transport}")
                add_test(NAME "${test_name}"
                    COMMAND "${disvm_command}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/flashvm/mixed_pair.py"
                        ${disvm_common} --source "${STYXRPC_DISVM_SOURCE}" --player "${STYXRPC_FLASHVM_PLAYER}"
                        ${tool_args} ${mode_args} ${transport_args} "--cancel-${profile}")
                set_tests_properties("${test_name}" PROPERTIES TIMEOUT 70)
            endforeach()
        endforeach()
    endforeach()
endif()

foreach(mode IN ITEMS interpreter jit)
    set(mode_args)
    if(mode STREQUAL "jit")
        list(APPEND mode_args --jit)
    endif()
    foreach(transport IN ITEMS unix tcp)
        set(transport_args)
        if(transport STREQUAL "tcp")
            list(APPEND transport_args --tcp)
        endif()
        add_test(NAME "styxrpc-disvm-deferred-${mode}-${transport}"
            COMMAND "${disvm_command}" "${disvm_examples}/deferred.py" ${disvm_common}
                --device "$<TARGET_FILE:styxrpc-deferred-device>" --module "${disvm_upstream_modules}"
                --native-module "${disvm_native_modules}" ${mode_args} ${transport_args})
        set_tests_properties("styxrpc-disvm-deferred-${mode}-${transport}" PROPERTIES TIMEOUT 45)
        set(test_name "styxrpc-disvm-channel-admission-${mode}-${transport}")
        add_test(NAME "${test_name}"
            COMMAND "${disvm_command}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/flashvm/channel_admission.py" ${disvm_common}
                --source "${STYXRPC_DISVM_SOURCE}" --driver "$<TARGET_FILE:styxrpc-routed-client>"
                ${mode_args} ${transport_args})
        set_tests_properties("${test_name}" PROPERTIES TIMEOUT 65)
    endforeach()
    add_test(NAME styxrpc-disvm-mutations-${mode}
        COMMAND "${disvm_command}" "${disvm_examples}/mutations.py" ${disvm_common}
            --module "${disvm_upstream_modules}" --device "$<TARGET_FILE:styxrpc-mutable-device>"
            ${mode_args})
    set_tests_properties(styxrpc-disvm-mutations-${mode} PROPERTIES TIMEOUT 50)
    add_test(NAME styxrpc-disvm-load-${mode}
        COMMAND "${disvm_command}" "${disvm_examples}/run.py" ${disvm_common}
            --module "${disvm_upstream_modules}" --device "$<TARGET_FILE:styxrpc-device>"
            ${mode_args})
    add_test(NAME styxrpc-disvm-shared-cell-${mode}
        COMMAND "${disvm_command}" "${CMAKE_CURRENT_SOURCE_DIR}/examples/shared-cell/run.py"
            --host "$<TARGET_FILE:styxrpc-shared-cell>" --disvm "${disvm_runtime}"
            --limbo "${disvm_limbo}" --system-modules "${disvm_upstream_modules}"
            --native-modules "${disvm_native_modules}" --limbo-sources "${STYXRPC_DISVM_SOURCE}/limbo"
            ${mode_args})
    set_tests_properties(styxrpc-disvm-shared-cell-${mode} PROPERTIES TIMEOUT 45)
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
        add_test(NAME "styxrpc-flashvm-launch-${mode}-tcp"
            COMMAND "${disvm_command}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/flashvm/run.py"
                ${disvm_common} --source "${STYXRPC_DISVM_SOURCE}"
                --flash-host "${STYXRPC_FLASHVM_TEST_HOST}" --direction launch --tcp ${mode_args})
        set_tests_properties("styxrpc-flashvm-launch-${mode}-tcp" PROPERTIES TIMEOUT 45)
        add_test(NAME "styxrpc-flashvm-library-${mode}-tcp"
            COMMAND "${disvm_command}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/flashvm/run.py"
                ${disvm_common} --source "${STYXRPC_DISVM_SOURCE}"
                --flash-host "${STYXRPC_FLASHVM_TEST_HOST}" --direction flash-to-dis --module-load --tcp ${mode_args})
        set_tests_properties("styxrpc-flashvm-library-${mode}-tcp" PROPERTIES TIMEOUT 45)
    endforeach()
    add_test(NAME styxrpc-flashvm-vfs
        COMMAND "${disvm_command}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/flashvm/run.py"
            ${disvm_common} --source "${STYXRPC_DISVM_SOURCE}"
            --flash-host "${STYXRPC_FLASHVM_TEST_HOST}" --direction vfs --device "$<TARGET_FILE:styxrpc-device>")
    set_tests_properties(styxrpc-flashvm-vfs PROPERTIES TIMEOUT 45)
endif()


foreach(presentation IN ITEMS player window terminal terminal-window debug)
    if(presentation MATCHES "window$")
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
        if(presentation STREQUAL "player" OR presentation STREQUAL "debug")
            add_test(NAME "${test_name}-tcp"
                COMMAND "${disvm_command}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/flashvm/run.py"
                    ${disvm_common} --source "${STYXRPC_DISVM_SOURCE}" --flash-host "${paired_player}"
                    --direction "native-${presentation}" --tcp ${mode_args})
            set_tests_properties("${test_name}-tcp" PROPERTIES TIMEOUT 45)
        endif()
        if(presentation MATCHES "window$")
            set_tests_properties("${test_name}" PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy")
        endif()
    endforeach()
endforeach()
