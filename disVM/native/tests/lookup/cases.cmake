# Copyright (c) 2026 bhgv
# EN: Native lookup classification against independent original-9P wire vectors.
# RU: Native lookup classification против independent original-9P wire vectors.
add_executable(styx-lookup-peer tests/lookup/client.c)
target_link_libraries(styx-lookup-peer PRIVATE styx_client)
foreach(lookup_case inferno opaque denied unknown prefix empty deep rooted partial drop wrongtag cancel race timeout lateerror cloneerror)
    add_test(NAME styx-lookup-${lookup_case} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/lookup/oracle.py" "$<TARGET_FILE:styx-lookup-peer>" ${lookup_case})
    set_tests_properties(styx-lookup-${lookup_case} PROPERTIES TIMEOUT 15)
endforeach()
