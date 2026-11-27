# Copyright (c) 2026 bhgv
# EN: Actual native control cancellation over original C Styx TCP with reply-order gates.
# RU: Настоящая native control cancellation через original C Styx TCP с reply-order gates.
add_executable(control-cancel-server "${CMAKE_CURRENT_LIST_DIR}/server.c")
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/server.c" PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(control-cancel-server PRIVATE infernostyx)
add_executable(control-cancel-client "${CMAKE_CURRENT_LIST_DIR}/client.c")
target_link_libraries(control-cancel-client PRIVATE styx_client)


# EN/RU: Register operation/mode as one bounded real network case; no guest scheduling.
# Зарегистрировать operation/mode как bounded real network case, без guest scheduling.
function(control_cancel_case operation mode)
    add_test(NAME native-control-${operation}-${mode} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/wire.py"
        "$<TARGET_FILE:control-cancel-client>" "$<TARGET_FILE:control-cancel-server>" "${operation}" "${mode}")
    set_tests_properties(native-control-${operation}-${mode} PROPERTIES TIMEOUT 30)
endfunction()
foreach(operation walk open stat)
    foreach(mode success cancel race racefail precancel timeout)
        control_cancel_case(${operation} ${mode})
    endforeach()
endforeach()
foreach(operation create remove)
    foreach(mode success cancel race precancel error)
        control_cancel_case(${operation} ${mode})
    endforeach()
endforeach()
foreach(mode first detach from-success from-cancel partial)
    control_cancel_case(walk ${mode})
endforeach()
control_cancel_case(open error)

add_executable(control-cancel-refusal "${CMAKE_CURRENT_LIST_DIR}/failure.c")
target_link_libraries(control-cancel-refusal PRIVATE styx_client)
add_test(NAME native-control-cleanup-refusal COMMAND control-cancel-refusal)
set_tests_properties(native-control-cleanup-refusal PROPERTIES TIMEOUT 10)
foreach(control 1 2)
    if(control EQUAL 1)
        set(mode stepfail)
    else()
        set(mode between)
    endif()
    add_executable(control-cancel-${mode} "${CMAKE_CURRENT_LIST_DIR}/client.c" "${CMAKE_CURRENT_LIST_DIR}/step_failure.c")
    target_compile_definitions(control-cancel-${mode} PRIVATE CONTROL=${control})
    target_link_libraries(control-cancel-${mode} PRIVATE styx_client)
    add_test(NAME native-control-walk-${mode} COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/wire.py"
        "$<TARGET_FILE:control-cancel-${mode}>" "$<TARGET_FILE:control-cancel-server>" walk "${mode}")
    set_tests_properties(native-control-walk-${mode} PROPERTIES TIMEOUT 30)
endforeach()

foreach(operation open stat)
    add_test(NAME native-control-${operation}-single-slot COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/wire.py"
        "$<TARGET_FILE:control-cancel-client>" "$<TARGET_FILE:control-cancel-server>" "${operation}" cancel)
    set_tests_properties(native-control-${operation}-single-slot PROPERTIES TIMEOUT 30 ENVIRONMENT "STYX_CONTROL_SLOTS=1")
endforeach()
