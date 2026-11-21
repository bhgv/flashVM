# Copyright (c) 2026 bhgv
# EN: Submit through the bound program path and wait for the provider outcome.
# RU: Запустить по привязанному пути и дождаться результата провайдера.
implement Client;
include "sys.m";
include "vfsnative.m";

Client: module { init: fn(); };


# EN: Poll the one-shot result; empty means pending and read failure means transport failure.
# RU: Опросить результат; пустое чтение означает ожидание, ошибка чтения означает сбой связи.
init()
{
    sys := load Sys Sys->PATH;
    vfs := load Vfsnative Vfsnative->PATH;
    status := sys->open("/peer/.exit", Sys->OREAD);
    if (status == nil || sys->bind("/peer/implementation.dis", "/alias.dis", Sys->MREPL) < 0 ||
        vfs->run("/alias.dis") != 1) {
        raise "could not submit chain launch";
    }
    bytes := array[16] of byte;
    for (i := 0; i < 500; i++) {
        n := sys->read(status, bytes, len bytes);
        if (n < 0) {
            raise "chain completion transport failed";
        }
        if (n > 0) {
            if (string bytes[0:n] != "ok\n") {
                raise "chain owner reported failure";
            }
            sys->print("PASS mounted Styx launch completion\n");
            return;
        }
        sys->sleep(10);
    }
    raise "chain completion timed out";
}
