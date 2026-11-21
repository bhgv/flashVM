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
    target_sources(disvm PRIVATE terminal_launch/cli.c terminal_launch/config.c terminal_launch/mount.c terminal_launch/serve.c terminal_launch/library.c)
    target_compile_definitions(disvm PRIVATE DVM_TERMINAL_LAUNCH=1)
    set_property(SOURCE terminal_launch/cli.c terminal_launch/config.c terminal_launch/mount.c terminal_launch/serve.c terminal_launch/library.c APPEND PROPERTY INCLUDE_DIRECTORIES
        "${root}/disVM/native/client" "${root}/disVM/native/terminal"
        "${root}/disVM/native/terminal/order" "${root}/disVM/native/vfs"
        "${root}/disVM/native/launch" "${CMAKE_CURRENT_SOURCE_DIR}/vfs"
        "${root}/disVM/host/traversal" "${root}/remote_phy")
    target_link_libraries(disvm PRIVATE "-Wl,--start-group" ${terminal_launch_libraries} "-Wl,--end-group")
endif()
