# Copyright (c) 2026 bhgv
# EN: Repeat launches through the standalone Styx server.
# RU: Повторные запуски через самостоятельный сервер Styx.
implement Persistentcli;

include "sys.m";
include "vfsnative.m";

Persistentcli: module { init: fn(); };

init()
{
    sys := load Sys Sys->PATH;
    run_shared(sys);
    sys->print("PASS standalone persistent Styx launch\n");
}

include "styx-launch-client.b";
