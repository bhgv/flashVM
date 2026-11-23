# Copyright (c) 2026 bhgv
# EN: Register only supported native execution; old reference registrations stay archived.
# RU: Регистрировать лишь поддержанное native-исполнение; старые reference-проверки архивны.
if(DVM_RUNTIME_TESTS)
    enable_testing()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/native_arch_cases.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/register_native_tests.py")
    # EN/RU: EditorTargets may register historical probes; do not run unported hosts.
    # EditorTargets может регистрировать старые пробы; неперенесённые host не запускать.
    get_property(historical_tests DIRECTORY PROPERTY TESTS)
    if(historical_tests)
        set_tests_properties(${historical_tests} PROPERTIES DISABLED TRUE LABELS "archived")
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
        execute_process(COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/register_native_tests.py"
            RESULT_VARIABLE native_registration_status OUTPUT_VARIABLE native_registration)
        if(NOT native_registration_status EQUAL 0)
            message(FATAL_ERROR "Cannot register mandatory native acceptance")
        endif()
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/native-only-tests.cmake" "${native_registration}")
        include("${CMAKE_CURRENT_BINARY_DIR}/native-only-tests.cmake")
        add_test(NAME disvm_native_styx_generated_call COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test_styx_generated_call.py" "$<TARGET_FILE:disvm>"
            "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
        set_tests_properties(disvm_native_styx_generated_call PROPERTIES TIMEOUT 120 LABELS "native")
        add_test(NAME disvm_native_styx_managed_load COMMAND "${PYTHON_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../examples/remote-scalar/managed-fid/run.py"
            "$<TARGET_FILE:disvm>" "$<TARGET_FILE:limbo-bootstrap>")
        set_tests_properties(disvm_native_styx_managed_load PROPERTIES TIMEOUT 120 LABELS "native")
        if(DVM_TERMINAL_LAUNCH_SDK)
            set(chain_example "${CMAKE_CURRENT_SOURCE_DIR}/../examples/remote-scalar/run_chain.py")
            set(channel_example "${CMAKE_CURRENT_SOURCE_DIR}/../examples/remote-scalar/run_channel.py")
            add_test(NAME disvm_native_styx_channel_tcp_interpreter COMMAND "${PYTHON_EXECUTABLE}"
                "${channel_example}" --runtime "$<TARGET_FILE:disvm>" --limbo "$<TARGET_FILE:limbo-bootstrap>")
            add_test(NAME disvm_native_styx_channel_tcp_jit COMMAND "${PYTHON_EXECUTABLE}"
                "${channel_example}" --runtime "$<TARGET_FILE:disvm>" --limbo "$<TARGET_FILE:limbo-bootstrap>" --jit)
            set_tests_properties(disvm_native_styx_channel_tcp_interpreter disvm_native_styx_channel_tcp_jit
                PROPERTIES TIMEOUT 120 LABELS "native")
            set(channel_pair_example "${CMAKE_CURRENT_SOURCE_DIR}/../examples/remote-scalar/run_channel_pair.py")
            add_test(NAME disvm_native_styx_channel_pair_interpreter COMMAND "${PYTHON_EXECUTABLE}"
                "${channel_pair_example}" --runtime "$<TARGET_FILE:disvm>" --limbo "$<TARGET_FILE:limbo-bootstrap>")
            add_test(NAME disvm_native_styx_channel_pair_jit COMMAND "${PYTHON_EXECUTABLE}"
                "${channel_pair_example}" --runtime "$<TARGET_FILE:disvm>" --limbo "$<TARGET_FILE:limbo-bootstrap>" --jit)
            set_tests_properties(disvm_native_styx_channel_pair_interpreter disvm_native_styx_channel_pair_jit
                PROPERTIES TIMEOUT 120 LABELS "native")
            # EN: Isolate mixed selection and post-return channel lifetimes on TCP.
            # RU: Проверять смешанный ALT и каналы после возврата отдельно через TCP.
            set(channel_lifetime_example "${CMAKE_CURRENT_SOURCE_DIR}/../examples/remote-scalar/run_channel_lifetime.py")
            foreach(channel_case mixed result)
                add_test(NAME disvm_native_styx_channel_${channel_case}_interpreter COMMAND "${PYTHON_EXECUTABLE}"
                    "${channel_lifetime_example}" ${channel_case} --runtime "$<TARGET_FILE:disvm>" --limbo "$<TARGET_FILE:limbo-bootstrap>")
                add_test(NAME disvm_native_styx_channel_${channel_case}_jit COMMAND "${PYTHON_EXECUTABLE}"
                    "${channel_lifetime_example}" ${channel_case} --runtime "$<TARGET_FILE:disvm>" --limbo "$<TARGET_FILE:limbo-bootstrap>" --jit)
                set_tests_properties(disvm_native_styx_channel_${channel_case}_interpreter disvm_native_styx_channel_${channel_case}_jit
                    PROPERTIES TIMEOUT 120 LABELS "native")
            endforeach()
            add_test(NAME disvm_native_styx_chain_tcp_interpreter COMMAND "${PYTHON_EXECUTABLE}"
                "${chain_example}" --runtime "$<TARGET_FILE:disvm>" --limbo "$<TARGET_FILE:limbo-bootstrap>" local)
            add_test(NAME disvm_native_styx_chain_tcp_jit COMMAND "${PYTHON_EXECUTABLE}"
                "${chain_example}" --runtime "$<TARGET_FILE:disvm>" --limbo "$<TARGET_FILE:limbo-bootstrap>" --jit local)
            set_tests_properties(disvm_native_styx_chain_tcp_interpreter disvm_native_styx_chain_tcp_jit
                PROPERTIES TIMEOUT 120 LABELS "native")
        endif()
        # EN/RU: Keep intentional broken-code controls native; no reference execution.
        # Намеренные broken-code контроли остаются native, без reference-исполнения.
        foreach(control retirement_storage cancel_generation killwait_seal owner_monitor remote_managed guest_proxy network_proxy proxy_churn routed_results proxy_retirement proxy_handoffs)
            set(control_flags)
            if(control STREQUAL "network_proxy" OR control STREQUAL "proxy_churn" OR control STREQUAL "routed_results" OR control STREQUAL "proxy_retirement" OR control STREQUAL "proxy_handoffs")
                list(APPEND control_flags --controls-only)
            endif()
            add_test(NAME disvm_control_${control} COMMAND "${PYTHON_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/test_${control}.py" "$<TARGET_FILE:disvm>"
                "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" ${control_flags})
            # EN/RU: Whole-network controls include many separate bounded real-VM cases under TSan.
            # Whole-network controls содержат много отдельных bounded real-VM cases под TSan.
            set(control_budget 180)
            if(control STREQUAL "network_proxy")
                set(control_budget 360)
            endif()
            set_tests_properties(disvm_control_${control} PROPERTIES TIMEOUT ${control_budget} LABELS "native")
        endforeach()
        if(CMAKE_C_FLAGS MATCHES "-fsanitize=thread")
            add_test(NAME disvm_control_namespace_scalar COMMAND "${PYTHON_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/test_namespace.py" "$<TARGET_FILE:disvm>"
                "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --controls-only)
            set_tests_properties(disvm_control_namespace_scalar PROPERTIES TIMEOUT 120 LABELS "native")
            add_test(NAME disvm_control_late_reply_race COMMAND "${PYTHON_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/test_late_reply.py" "$<TARGET_FILE:disvm>"
                "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --race-control)
            set_tests_properties(disvm_control_late_reply_race PROPERTIES TIMEOUT 180 LABELS "native")
        endif()
    endif()
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/UnibusTests.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ProviderModuleTests.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/LaunchContextTests.cmake")
