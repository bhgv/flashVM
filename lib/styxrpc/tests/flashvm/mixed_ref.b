# Copyright (c) 2026 bhgv
# EN: Receive, forward and mutate the typed Int32 cell through a stock DisVM mount.
# RU: Принять, переслать и изменить типизированную ячейку Int32 через штатный маунт DisVM.
implement Client;
include "sys.m";
include "styxintref.m";
include "styxchannel.m";

Client: module { init: fn(); };
sys: Sys; # Timer and diagnostics.
rpc: Styxchannel; # Typed exchange client.
refs: Styxintref; # Module that implements Cell methods.
Port, Message: import rpc;
Cell: import refs;


# EN: Poll the exact port for one typed reference, yielding between empty polls.
# RU: Опросить конкретный порт для типизированной ссылки, с паузами при ожидании.
receive(port: ref Port): ref Cell
{
    for (i := 0; i < 150; ++i) {
        incoming := port.recv();
        if (incoming != nil) {
            pick item := incoming {
            Variable => return item.cell;
            * => raise "unexpected mixed reference kind";
            }
        }
        sys->sleep(10);
    }
    raise "mixed reference timed out";
}


# EN: Prove two separately accepted descriptors share one server-owned cell.
# RU: Проверить, что два отдельно принятых дескриптора ведут к одной ячейке сервера.
init()
{
    sys = load Sys Sys->PATH;
    rpc = load Styxchannel Styxchannel->PATH;
    refs = load Styxintref Styxintref->PATH;
    rpc->init();
    refs->init();
    port := rpc->newchan("/.rpc", "/rpc");
    first := receive(port);
    if (first.read() != 41 || !port.send(ref Message.Variable(first))) {
        raise "mixed reference send failed";
    }
    second := receive(port);
    port.close();
    first.write(-32);
    if (second.read() != -32) {
        raise "mixed reference aliases diverged";
    }
    sys->print("PASS Limbo live Int32 reference over mixed exchange\n");
}
