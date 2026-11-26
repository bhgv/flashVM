# Copyright (c) 2026 bhgv
# EN: Optional hosted VM bridge using an independently installed StyxRpc SDK.
# RU: Хостовый мост VM с отдельно установленным SDK StyxRpc.
option(FVM_ENABLE_STYXRPC "Build the hosted libstyxrpc bridge" OFF)
if(FVM_ENABLE_STYXRPC)
    find_package(StyxRpc CONFIG REQUIRED)
    if(NOT TARGET StyxRpc::launch OR NOT TARGET StyxRpc::serve)
        message(FATAL_ERROR "The FlashVM bridge requires a hosted StyxRpc SDK")
    endif()
    add_library(flashvm_styxrpc STATIC src/styxrpc/scalar.c src/styxrpc/managed.c src/styxrpc/managed_values.c src/styxrpc/managed_refs.c src/styxrpc/ref_slots.cpp
        src/styxrpc/graph.c src/styxrpc/graph_refs.c src/styxrpc/hybrid.c src/styxrpc/hybrid_handle.cpp
        src/styxrpc/channel_import.c src/styxrpc/channel_methods.c src/styxrpc/channel_values.c src/styxrpc/channel_delegate.c src/styxrpc/channel_handle.cpp
        src/styxrpc/channel_export.c src/styxrpc/channel_output.c src/styxrpc/channel_local.c src/styxrpc/channel_lend.c src/styxrpc/channel_handler.c
        src/styxrpc/channel_local_state.c src/styxrpc/channel_publication.c src/styxrpc/channel_mailbox.c
        src/styxrpc/channel_pid_mailbox.c src/styxrpc/channel_select.c
        src/styxrpc/session.c src/styxrpc/session_launch.c src/styxrpc/session_vfs.c src/styxrpc/session_paths.c src/styxrpc/session_calls.c src/styxrpc/session_exec.c
        src/styxrpc/session_modules.c src/styxrpc/session_messages.c src/styxrpc/session_channel.c src/styxrpc/session_named_channel.cpp src/styxrpc/session_named_values.cpp src/styxrpc/session_named_refs.cpp
        src/styxrpc/session_named_snapshot.cpp src/styxrpc/session_module_refs.cpp src/styxrpc/session_nodes.cpp
        src/styxrpc/filesystem.c src/styxrpc/variable.c src/styxrpc/variable_ref_adapter.c src/styxrpc/variable_ref_identity.cpp src/styxrpc/variable_ref_channel.cpp src/styxrpc/object_image.c src/styxrpc/module_import.c
        src/styxrpc/playback.c src/styxrpc/player_services.c src/styxrpc/movie_load.c src/styxrpc/movie_requests.c
        src/styxrpc/os.c src/styxrpc/os_open.c ports/styxrpc_stats_port.c)
    target_link_libraries(flashvm_styxrpc PUBLIC flashvm StyxRpc::launch StyxRpc::serve)
    if(UNIX AND NOT ANDROID AND TARGET Styx::bidir)
        add_executable(flashvm_styx_player src/styxrpc/player_main.c src/styxrpc/player_view.c
            src/styxrpc/player_terminal.c src/styxrpc/player_peer.c)
        if(FVM_ENABLE_AGG OR FVM_ENABLE_CAIRO)
            target_compile_definitions(flashvm_styx_player PRIVATE FVM_STYXRPC_RASTER)
        endif()
        target_link_libraries(flashvm_styx_player PRIVATE flashvm_styxrpc Styx::bidir)
        install(TARGETS flashvm_styx_player RUNTIME DESTINATION bin)
    endif()
    if(FVM_BUILD_TESTS)
        add_executable(flashvm_tests_styxrpc_host tests/styxrpc_host.c tests/styxrpc/managed.c
            tests/styxrpc/launch.c tests/styxrpc/vfs.c tests/styxrpc/refs.c tests/styxrpc/graph.c
            tests/styxrpc/hybrid.c tests/styxrpc/channel.c tests/styxrpc/channel-service.c tests/styxrpc/channel-args.c
            tests/styxrpc/channel-incoming.c tests/styxrpc/channel-delegated.c tests/styxrpc/channel-pid-graph.c
            tests/styxrpc/channel-mailbox.c tests/styxrpc/channel-pid-result.c tests/styxrpc/object-module.c
            tests/styxrpc/playback.c tests/styxrpc/movie-assets.c)
        target_link_libraries(flashvm_tests_styxrpc_host PRIVATE flashvm_styxrpc Styx::bidir)
        set_property(TARGET flashvm_tests_styxrpc_host PROPERTY C_STANDARD 99)
        if(UNIX AND NOT ANDROID AND TARGET Styx::bidir)
            add_executable(flashvm_tests_styxrpc_node_files tests/styxrpc/node-files.c tests/styxrpc/movie-assets.c)
            target_link_libraries(flashvm_tests_styxrpc_node_files PRIVATE flashvm_styxrpc Styx::bidir)
            set_property(TARGET flashvm_tests_styxrpc_node_files PROPERTY C_STANDARD 99)
            add_test(NAME flashvm_tests_styxrpc_node_files COMMAND flashvm_tests_styxrpc_node_files)
            set_tests_properties(flashvm_tests_styxrpc_node_files PROPERTIES TIMEOUT 10)
            add_executable(flashvm_tests_styxrpc_variable_ref tests/styxrpc/variable-ref.c tests/styxrpc/named-ref.cpp)
            target_link_libraries(flashvm_tests_styxrpc_variable_ref PRIVATE flashvm_styxrpc Styx::bidir)
            set_property(TARGET flashvm_tests_styxrpc_variable_ref PROPERTY C_STANDARD 99)
            add_test(NAME flashvm_tests_styxrpc_variable_ref COMMAND flashvm_tests_styxrpc_variable_ref)
            set_tests_properties(flashvm_tests_styxrpc_variable_ref PROPERTIES TIMEOUT 10)
        endif()
    endif()
endif()
