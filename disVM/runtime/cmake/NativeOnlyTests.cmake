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
        # EN/RU: Keep intentional broken-code controls native; no reference execution.
        # Намеренные broken-code контроли остаются native, без reference-исполнения.
        foreach(control retirement_storage cancel_generation killwait_seal owner_monitor remote_managed guest_proxy network_proxy proxy_churn routed_results proxy_retirement)
            set(control_flags)
            if(control STREQUAL "network_proxy" OR control STREQUAL "proxy_churn" OR control STREQUAL "routed_results" OR control STREQUAL "proxy_retirement")
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
            add_test(NAME disvm_control_late_reply_race COMMAND "${PYTHON_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/test_late_reply.py" "$<TARGET_FILE:disvm>"
                "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --race-control)
            set_tests_properties(disvm_control_late_reply_race PROPERTIES TIMEOUT 180 LABELS "native")
        endif()
    endif()
endif()
