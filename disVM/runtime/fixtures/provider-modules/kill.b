# Copyright (c) 2026 bhgv
# EN: Kill a real child blocked in provider module read, using an ordinary provider FD readiness barrier.
# RU: Завершить настоящего child в provider module read через обычный FD-barrier готовности провайдера.
implement Check;
include "sys.m";
include "value.m";
include "processctl.m";
Check: module
{
    init: fn(); # Existing console bootstrap ABI.
};


# EN/RU: Publish PID before entering the real provider loader; execution must not resume past accepted killwait.
# Сообщить PID до настоящего provider loader; после принятого killwait исполнение не продолжается.
child(ready: chan of int)
{
    ctl := load Processctl Processctl->PATH;
    ready <-= ctl->pid();
    ignored := load Value "/dep.dis";
    raise "provider loader child resumed after kill";
}


# EN/RU: Wait for actual native read admission via FD, then kill only that child and keep ordinary data usable.
# Дождаться настоящего native read через FD, завершить лишь этого child и сохранить обычный data-access.
init()
{
    sys := load Sys Sys->PATH;
    ctl := load Processctl Processctl->PATH;
    ready := chan of int;
    spawn child(ready);
    pid := <-ready;
    gate := sys->open("/waiting", Sys->OREAD);
    bytes := array[32] of byte;
    n := sys->read(gate, bytes, len bytes);
    if (n != 5 || string bytes[:n] != "ready" || ctl->killwait(pid) != Processctl->Retired) {
        raise "provider native killwait failed";
    }
    data := sys->open("/data", Sys->OREAD);
    n = sys->read(data, bytes, len bytes);
    if (n != 8 || string bytes[:n] != "terminal") {
        raise "parent lost namespace after child loader kill";
    }
    sys->print("PASS provider-backed Dis module and dependency\n");
}
