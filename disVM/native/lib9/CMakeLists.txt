# Copyright (c) 2026 bhgv
# EN: Original complete POSIX lib9 archive with explicit shared native thread adapters.
# RU: Полная POSIX-библиотека lib9 с явными адаптерами native-потоков.
execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/sources.py"
    "${INFERNO_SOURCE}/lib9" "${inferno_arch}" RESULT_VARIABLE status OUTPUT_VARIABLE names OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Cannot read original Inferno lib9 source selection")
endif()
set(sources)
foreach(name ${names})
    if(name STREQUAL "fmt.c" OR name STREQUAL "errstr-posix.c")
        if(name STREQUAL "fmt.c")
            set(unit fmt)
        else()
            set(unit errstr)
        endif()
        set(prepared "${CMAKE_CURRENT_BINARY_DIR}/${name}")
        execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${INFERNO_HOST_SUPPORT}/prepare_${unit}.py"
            --source "${INFERNO_SOURCE}/lib9/${name}" --output "${prepared}" RESULT_VARIABLE status)
        if(NOT status EQUAL 0)
            message(FATAL_ERROR "Native lib9 ${unit} preparation failed")
        endif()
        list(APPEND sources "${prepared}")
    elseif(name STREQUAL "fmtlock.c")
        list(APPEND sources "${CMAKE_CURRENT_SOURCE_DIR}/../ports/posix/fmtlock.c")
    else()
        list(APPEND sources "${INFERNO_SOURCE}/lib9/${name}")
    endif()
endforeach()
set(dtoa "${CMAKE_CURRENT_BINARY_DIR}/dtoa.c")
execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${INFERNO_HOST_SUPPORT}/prepare_dtoa.py"
    --source "${INFERNO_SOURCE}/libmath/dtoa.c" --output "${dtoa}" RESULT_VARIABLE status)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Native lib9 decimal support preparation failed")
endif()
add_library(inferno9 STATIC ${sources} "${dtoa}" "${INFERNO_SOURCE}/libmath/FPcontrol-Linux.c"
    "${INFERNO_SOURCE}/libmath/g_fmt.c" "${INFERNO_SOURCE}/libmath/gfltconv.c"
    "${INFERNO_HOST_SUPPORT}/host/decimal_sync.c" "${CMAKE_CURRENT_SOURCE_DIR}/../ports/posix/fatal.c")
target_include_directories(inferno9 PUBLIC ${inferno_headers} PRIVATE "${INFERNO_SOURCE}/lib9")
target_compile_options(inferno9 PRIVATE -fno-strict-aliasing -Wall -Wextra)
target_link_libraries(inferno9 PUBLIC Threads::Threads m)
set_target_properties(inferno9 PROPERTIES OUTPUT_NAME 9)
