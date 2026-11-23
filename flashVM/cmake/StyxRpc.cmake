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
        src/styxrpc/channel_local_state.c src/styxrpc/channel_publication.c
        src/styxrpc/session.c src/styxrpc/session_launch.c src/styxrpc/session_vfs.c)
    target_link_libraries(flashvm_styxrpc PUBLIC flashvm StyxRpc::launch StyxRpc::serve)
    if(FVM_BUILD_TESTS)
        add_executable(flashvm_tests_styxrpc_host tests/styxrpc_host.c tests/styxrpc/managed.c
            tests/styxrpc/launch.c tests/styxrpc/vfs.c tests/styxrpc/refs.c tests/styxrpc/graph.c
            tests/styxrpc/hybrid.c tests/styxrpc/channel.c tests/styxrpc/channel-service.c tests/styxrpc/channel-args.c
            tests/styxrpc/channel-incoming.c tests/styxrpc/channel-delegated.c tests/styxrpc/channel-pid-graph.c)
        target_link_libraries(flashvm_tests_styxrpc_host PRIVATE flashvm_styxrpc Styx::bidir)
        set_property(TARGET flashvm_tests_styxrpc_host PROPERTY C_STANDARD 99)
    endif()
endif()
