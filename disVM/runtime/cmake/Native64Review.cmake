# Optional owning-source review gates are independent of the installed NC64 runner.
# They use the source-built compiler only for native64 declarations, never its NC64 output.
option(DVM_RUNTIME_NATIVE64_REVIEW_TESTS "Run strict-UBSan native64 ownership review gates" OFF)
if(DVM_RUNTIME_NATIVE64_REVIEW_TESTS)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Native64 review gates require a compiler with -fsanitize=undefined")
    endif()
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/native64-review")
    foreach(review module_sets typed_sys)
        if(review STREQUAL "module_sets")
            set(review_script "${u}/utils/tests/native64-builtins.py")
            set(review_options --sets)
        else()
            set(review_script "${u}/utils/tests/native64-arguments.py")
            set(review_options --formatter --limbo "$<TARGET_FILE:limbo-bootstrap>")
        endif()
        add_test(NAME disvm_native64_review_${review}
            COMMAND "${PYTHON_EXECUTABLE}" "${review_script}" ${review_options})
        set_tests_properties(disvm_native64_review_${review} PROPERTIES
            TIMEOUT 360
            ENVIRONMENT "NATIVE64_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/native64-review;CC=${CMAKE_C_COMPILER};NATIVE64_LIMBO=$<TARGET_FILE:limbo-bootstrap>;NATIVE64_CFLAGS=-O3 -g -DNDEBUG -fno-strict-aliasing -fsanitize=undefined -fno-sanitize-recover=all")
    endforeach()
endif()
