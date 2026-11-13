# Copyright (c) 2026 bhgv
# EN: Keep pure ABI/guard/generator checks without starting a reference VM.
# RU: Сохранить проверки ABI/guards/генератора без запуска reference VM.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    get_target_property(native_unit_includes disvm INCLUDE_DIRECTORIES)
    list(APPEND native_unit_includes "${DVM_FLASHVM_SOURCE_ROOT}/ports" "${DVM_FLASHVM_SOURCE_ROOT}/include")
    add_executable(disvm_sys_capture_test test_sys_capture.c sys_capture.c "${root}/disVM/host/refcount.c")
    add_dependencies(disvm_sys_capture_test disvm_runtime_sources)
    target_include_directories(disvm_sys_capture_test PRIVATE ${native_unit_includes})
    target_compile_definitions(disvm_sys_capture_test PRIVATE DVM_NATIVE_REGISTERS=1)
    add_executable(disvm_sys_owner_race_test test_sys_owner_race.c sys_capture.c "${root}/disVM/host/refcount.c")
    add_dependencies(disvm_sys_owner_race_test disvm_runtime_sources)
    target_include_directories(disvm_sys_owner_race_test PRIVATE ${native_unit_includes})
    target_compile_definitions(disvm_sys_owner_race_test PRIVATE DVM_NATIVE_REGISTERS=1)
    target_link_libraries(disvm_sys_owner_race_test PRIVATE Threads::Threads)
    add_test(NAME disvm_sys_owner_race COMMAND disvm_sys_owner_race_test)
    set_tests_properties(disvm_sys_owner_race PROPERTIES TIMEOUT 30 LABELS "host-unit")
    foreach(capture_case RANGE 0 18)
        add_test(NAME disvm_sys_capture_${capture_case} COMMAND disvm_sys_capture_test ${capture_case})
        set_tests_properties(disvm_sys_capture_${capture_case} PROPERTIES TIMEOUT 10 LABELS "host-unit")
    endforeach()
    add_test(NAME disvm_runtime_platform_selection COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_platform.py" "${CMAKE_COMMAND}"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RuntimePlatform.cmake")
    set_tests_properties(disvm_runtime_platform_selection PROPERTIES TIMEOUT 30 LABELS "host-unit")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
    add_test(NAME disvm_native_generation_idempotent COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_generation.py" "$<TARGET_FILE:limbo-bootstrap>"
        "${u}/libinterp" "${view}")
    set_tests_properties(disvm_native_generation_idempotent PROPERTIES TIMEOUT 30 LABELS "host-unit"
        ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
endif()
