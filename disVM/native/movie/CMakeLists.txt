# Copyright (c) 2026 bhgv
# EN: Managed movie SDK uses explicit repository VFS/FlashVM protocol inputs and original Styx client.
# RU: Managed movie SDK использует explicit VFS/FlashVM protocol inputs и original Styx client.
set(movie_flash "${INFERNO_VFS_SUPPORT}/flashVM")
add_library(styx_movie_protocol STATIC announcement.c protocol.c
    "${movie_flash}/src/protocol/srecord.c" "${movie_flash}/src/protocol/sbindings.c")
target_include_directories(styx_movie_protocol PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${movie_flash}/include")
add_library(styx_movie_service STATIC root.c offer.c publisher.c writer.c service.c request.c)
target_link_libraries(styx_movie_service PUBLIC styx_namespace_fixture styx_movie_protocol)
add_library(styx_movie_client STATIC receiver.c client.c)
target_include_directories(styx_movie_client PUBLIC "${movie_flash}/frontend/common")
target_link_libraries(styx_movie_client PUBLIC styx_client styx_movie_protocol styx_vfs)
add_executable(movie-server tests/backend.c tests/server.c tests/deferred.c tests/control.c ../tests/file-root/context.c)
set_source_files_properties(tests/server.c tests/deferred.c PROPERTIES INCLUDE_DIRECTORIES "${inferno_headers}")
target_link_libraries(movie-server PRIVATE styx_movie_service infernostyx)

set(INFERNO_MOVIE_PLAYER_BUILD "" CACHE PATH "Explicit built FlashVM desktop SDK for movie acceptance")
if(INFERNO_MOVIE_PLAYER_BUILD)
    enable_language(CXX)
    find_package(SDL2 REQUIRED)
    add_executable(movie-player tests/player.c tests/announced.c tests/watch_delay.c)
    set_property(TARGET movie-player APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=styxclientwatch")
    target_link_libraries(movie-player PRIVATE styx_movie_client
        "${INFERNO_MOVIE_PLAYER_BUILD}/frontend/libflashvm_player_host.a"
        "${INFERNO_MOVIE_PLAYER_BUILD}/libflashvm.a"
        "${INFERNO_MOVIE_PLAYER_BUILD}/libfvm_minimp3.a"
        "${INFERNO_MOVIE_PLAYER_BUILD}/libfvm_zlib.a"
        "${INFERNO_MOVIE_PLAYER_BUILD}/libfvm_agg.a" SDL2::SDL2 m)
    set_target_properties(movie-player PROPERTIES LINKER_LANGUAGE CXX)
endif()
if(INFERNO_MOVIE_PLAYER_BUILD)
    add_test(NAME managed-movie-wire COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/wire.py" "$<TARGET_FILE:movie-server>"
        "$<TARGET_FILE:movie-player>" "${movie_flash}")
    set_tests_properties(managed-movie-wire PROPERTIES TIMEOUT 100)
endif()
add_executable(movie-protocol-test tests/protocol.c ../tests/file-root/context.c)
target_link_libraries(movie-protocol-test PRIVATE styx_movie_service)
add_test(NAME managed-movie-protocol COMMAND movie-protocol-test)
set_tests_properties(managed-movie-protocol PROPERTIES TIMEOUT 10)

foreach(movie_target styx_movie_protocol styx_movie_service styx_movie_client)
    target_compile_options(${movie_target} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()

add_executable(movie-offers-test tests/offers.c ../tests/file-root/context.c)
target_link_libraries(movie-offers-test PRIVATE styx_movie_service)
add_test(NAME managed-movie-offers COMMAND movie-offers-test)
set_tests_properties(managed-movie-offers PROPERTIES TIMEOUT 10)

add_executable(movie-writer-test tests/writer.c ../tests/file-root/context.c)
target_link_libraries(movie-writer-test PRIVATE styx_movie_service)
add_test(NAME managed-movie-writer COMMAND movie-writer-test)
set_tests_properties(managed-movie-writer PROPERTIES TIMEOUT 10)

add_test(NAME managed-movie-automatic COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/automatic.py" "$<TARGET_FILE:movie-server>"
    "${INFERNO_SOURCE}/tools/libstyx/tests")
set_tests_properties(managed-movie-automatic PROPERTIES TIMEOUT 20)
