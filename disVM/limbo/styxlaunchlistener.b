# Copyright (c) 2026 bhgv
# EN: Standalone hosted launch listener. The runtime owns admission and completion.
# RU: Приёмник самостоятельного сервиса запуска. Допуском и итогом управляет VM.
implement Styxlaunchlistener;
include "vfsnative.m";

Styxlaunchlistener: module { init: fn(); };

init()
{
    routes := load Vfsnative Vfsnative->PATH;
    if (routes == nil)
        raise "Styx launch listener has no VFS runtime";
    routes->acceptsharedlaunch();
}
