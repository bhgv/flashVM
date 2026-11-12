# Copyright (c) 2026 bhgv
# EN: Concurrent collector acceptance on real native resident processes.
# RU: Приёмка конкурентной сборки на настоящих закреплённых процессах ОС.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_exception_retirement COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_exception_retirement.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_exception_retirement PROPERTIES TIMEOUT 90)
    add_test(NAME disvm_managed_retirement COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_managed_retirement.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_managed_retirement PROPERTIES TIMEOUT 90)
    add_test(NAME disvm_cycle_owner_rescue COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_cycle_owner_rescue.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_cycle_owner_rescue PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_module_retirement COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_module_retirement.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_module_retirement PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_packet_cycle_retirement COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_packet_cycle_retirement.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_packet_cycle_retirement PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_module_cycle_retirement COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_module_cycle_retirement.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_module_cycle_retirement PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_duke_pin_retirement COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_duke_pin_retirement.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_duke_pin_retirement PROPERTIES TIMEOUT 120)
    add_executable(disvm_fast_readiness_test test_fast_readiness.c)
    target_link_libraries(disvm_fast_readiness_test PRIVATE disvm_flashvm_host)
    set_target_properties(disvm_fast_readiness_test PROPERTIES LINK_FLAGS "-Wl,--wrap=dvm_world_try_quiescent")
    add_test(NAME disvm_fast_readiness COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_fast_readiness.py" "$<TARGET_FILE:disvm_fast_readiness_test>")
    set_tests_properties(disvm_fast_readiness PROPERTIES TIMEOUT 30)
endif()
