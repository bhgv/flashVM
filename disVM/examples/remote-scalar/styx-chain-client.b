# Copyright (c) 2026 bhgv
# EN: Submit through the bound program path and wait for the provider outcome.
# RU: Запустить по привязанному пути и дождаться результата провайдера.
implement Client;
include "sys.m";
include "vfsnative.m";

Client: module { init: fn(); };


# EN: Run the bound module and receive its result on the opened request.
# RU: Запустить забинженный модуль и получить итог на открытом запросе.
init()
{
    sys := load Sys Sys->PATH;
    vfs := load Vfsnative Vfsnative->PATH;
    if (sys->bind("/peer/implementation.dis", "/alias.dis", Sys->MREPL) < 0) {
        raise "could not bind chain launch";
    }
    case vfs->run("/alias.dis") {
        1 => sys->print("PASS mounted Styx launch completion\n");
        -2 => raise "chain owner reported failure";
        -1 => raise "chain completion transport failed";
        * => raise "could not submit chain launch";
    }
}
