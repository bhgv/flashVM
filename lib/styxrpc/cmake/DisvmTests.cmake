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
