# Copyright (c) 2026 bhgv
# EN: Link the optional one-shot launcher against the source-matched native SDK.
# RU: Связать необязательный лаунчер с нативным SDK той же версии.
set(DVM_TERMINAL_LAUNCH_SDK "" CACHE PATH "Build directory of the source-matched Linux native Styx SDK")
if(DVM_TERMINAL_LAUNCH_SDK)
    set(terminal_launch_archives
        terminal/order/files/libstyx_ordered_files.a
        terminal/libstyx_terminal_route.a
        terminal/order/broker/libstyx_terminal_ordered.a
        terminal/order/libstyx_terminal_order.a
        terminal/libstyx_terminal_ui.a
        terminal/libstyx_terminal.a
        terminal/libstyx_unibus_server.a
        server/phy/libinfernostyx_phy.a
        movie/libstyx_movie_protocol.a
        launch/libstyx_launch.a
        vfs/libstyx_namespace_fixture.a
        vfs/libstyx_vfs.a
        client/libstyx_client.a
        client/walk/libvfs_walk.a
        client/remote_phy/libremote_phy_mux.a
        client/remote_phy/libremote_phy.a
        lib9/lib9.a)
    set(terminal_launch_libraries)
    foreach(archive IN LISTS terminal_launch_archives)
        set(path "${DVM_TERMINAL_LAUNCH_SDK}/${archive}")
        if(NOT EXISTS "${path}")
            message(FATAL_ERROR "Missing launch SDK archive: ${path}. Build the terminal-ordered-files SDK target.")
        endif()
        list(APPEND terminal_launch_libraries "${path}")
    endforeach()
    set(launch_listener_source "${root}/disVM/limbo/styxlaunchlistener.b")
    set(launch_listener_embed "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedLaunchListener.cmake")
    set(launch_listener_dis "${CMAKE_CURRENT_BINARY_DIR}/styxlaunchlistener.dis")
    set(launch_listener_header "${CMAKE_CURRENT_BINARY_DIR}/styxlaunchlistener_image.h")
    add_custom_command(OUTPUT "${launch_listener_header}" BYPRODUCTS "${launch_listener_dis}"
        COMMAND "$<TARGET_FILE:limbo-bootstrap>" -I "${root}/disVM/module"
            -o "${launch_listener_dis}" "${launch_listener_source}"
        COMMAND "${CMAKE_COMMAND}" "-DINPUT=${launch_listener_dis}" "-DOUTPUT=${launch_listener_header}"
            -P "${launch_listener_embed}"
        DEPENDS limbo-bootstrap "${launch_listener_source}" "${root}/disVM/module/vfsnative.m"
            "${launch_listener_embed}" VERBATIM)
    set(live_owner_dis "${CMAKE_CURRENT_BINARY_DIR}/live-module-owner.dis")
    set(live_owner_header "${CMAKE_CURRENT_BINARY_DIR}/live_module_owner_image.h")
    file(GLOB live_owner_sources "${root}/disVM/limbo/scalarowner*.b")
    add_custom_command(OUTPUT "${live_owner_header}" BYPRODUCTS "${live_owner_dis}"
        COMMAND "$<TARGET_FILE:limbo-bootstrap>" -D RG -I "${root}/disVM/module"
            -I "${root}/disVM/upstream/module" -o "${live_owner_dis}" "${root}/disVM/limbo/scalarowner.b"
        COMMAND "${CMAKE_COMMAND}" "-DINPUT=${live_owner_dis}" "-DOUTPUT=${live_owner_header}"
            "-DSYMBOL=live_module_owner_image" -P "${launch_listener_embed}"
        DEPENDS limbo-bootstrap ${live_owner_sources} "${root}/disVM/module/vfsnative.m"
            "${launch_listener_embed}" VERBATIM)
    set(live_module_sources terminal_launch/live_modules.c terminal_launch/live_module_control.c
        terminal_launch/live_module_routes.c terminal_launch/live_module_guest.c vfs/live_module_owner.c)
    target_sources(disvm PRIVATE "${live_owner_header}" terminal_launch/live_modules.c
        terminal_launch/live_module_control.c terminal_launch/live_module_routes.c terminal_launch/live_module_guest.c)
    target_sources(disvm PRIVATE "${launch_listener_header}")
    target_include_directories(disvm PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_sources(disvm PRIVATE terminal_launch/cli.c terminal_launch/config.c terminal_launch/mount.c terminal_launch/serve.c terminal_launch/library.c terminal_launch/local_pair.c terminal_launch/mount_assets.c)
    target_compile_definitions(disvm PRIVATE DVM_TERMINAL_LAUNCH=1)
    set(launch_sources terminal_launch/cli.c terminal_launch/config.c terminal_launch/mount.c
        terminal_launch/serve.c terminal_launch/library.c ${live_module_sources})
    set(launch_includes
        "${root}/lib/styxrpc/include"
        "${root}/disVM/native/client" "${root}/disVM/native/terminal"
        "${root}/disVM/native/terminal/order" "${root}/disVM/native/vfs"
        "${root}/disVM/native/launch" "${CMAKE_CURRENT_SOURCE_DIR}/vfs"
        "${root}/disVM/host/traversal" "${root}/remote_phy")
    set(styxrpc_terminal_sources
        terminal_launch/paired_ui.c
        terminal_launch/paired_exchange.c
        terminal_launch/named_channels.c
        terminal_launch/module_refs.c
        terminal_launch/executable_nodes.c
        terminal_launch/exec_mailbox.c
        terminal_launch/exec_names.c
        "${root}/lib/styxrpc/src/node_service.c"
        "${root}/lib/styxrpc/src/named_channel.c"
        "${root}/lib/styxrpc/src/named_cell_store.c"
        "${root}/lib/styxrpc/src/named_cell_wire.c"
        "${root}/lib/styxrpc/src/named_channel_path.c"
        "${root}/lib/styxrpc/src/exchange.c"
        "${root}/lib/styxrpc/src/value_codec.c"
        "${root}/lib/styxrpc/src/unibus_terminal.c"
        "${root}/lib/styxrpc/src/message_port.c"
        "${root}/lib/styxrpc/src/message_client.c"
        "${root}/lib/styxrpc/src/vfs.c"
        "${root}/lib/styxrpc/src/vfs_client.c"
        "${root}/lib/styxrpc/protocol/module_ref.c"
        "${root}/lib/styxrpc/src/variable_ref_client.c"
        "${root}/lib/styxrpc/src/module_ref_port.c")
    target_sources(disvm PRIVATE ${styxrpc_terminal_sources})
    set(styxrpc_terminal_includes
        "${root}/lib/styxrpc/include" "${root}/lib/styxrpc/protocol"
        "${root}/disVM/native/client" "${root}/disVM/native/vfs"
        "${root}/disVM/native/terminal" "${root}/remote_phy")
    if(CMAKE_VERSION VERSION_LESS 3.11)
        # Keep these quoted headers ahead of the target includes, including its other client.h.
        foreach(group launch styxrpc_terminal)
            set(flags "")
            foreach(directory IN LISTS ${group}_includes)
                string(APPEND flags " -iquote \"${directory}\"")
            endforeach()
            set_property(SOURCE ${${group}_sources} APPEND_STRING PROPERTY COMPILE_FLAGS "${flags}")
        endforeach()
    else()
        set_property(SOURCE ${launch_sources} APPEND PROPERTY INCLUDE_DIRECTORIES ${launch_includes})
        set_property(SOURCE ${styxrpc_terminal_sources} APPEND PROPERTY INCLUDE_DIRECTORIES ${styxrpc_terminal_includes})
    endif()
    target_link_libraries(disvm PRIVATE "-Wl,--start-group" ${terminal_launch_libraries} "-Wl,--end-group")
endif()
