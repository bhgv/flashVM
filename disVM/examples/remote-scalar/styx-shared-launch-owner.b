# Copyright (c) 2026 bhgv
# EN: Exercise the configured application entry on the combined Styx server.
# RU: Проверить настроенную точку входа программы на общем сервере Styx.
implement Sharedlaunch;
include "sys.m";

Sharedlaunch: module { init: fn(); };


# EN: Verify the private context and record execution in the provider's VFS.
# RU: Проверить частный контекст и отметить исполнение в VFS провайдера.
init()
{
    sys := load Sys Sys->PATH;
    pid := sys->open("/.prog/.self/.pid", Sys->OREAD);
    data := array[32] of byte;
    if (pid == nil || sys->read(pid, data, len data) <= 0) {
        raise "combined launch has no caller PID";
    }
    output := sys->open("/launch-marker", Sys->OWRITE);
    if (output == nil || sys->fprint(output, "ran\n") != 4) {
        raise "could not record combined launch";
    }
    if (sys->open("/launch-fail", Sys->OREAD) != nil) {
        raise "requested combined launch failure";
    }
}
