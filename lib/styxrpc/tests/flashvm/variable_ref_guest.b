# Copyright (c) 2026 bhgv
# EN: Accept a reference payload in a Limbo channel and use two live aliases.
# RU: Принять дескриптор по каналу Limbo и использовать две живые привязки.
implement Client;
include "sys.m";
include "styxintref.m";

Client: module { init: fn(); };


# EN: Forward the same payload twice and verify the aliases access the issuer.
# RU: Переслать дескриптор дважды и проверить доступ к ячейке владельца.
init()
{
    sys := load Sys Sys->PATH;
    refs := load Styxintref Styxintref->PATH;
    Cell: import refs;
    refs->init();
    bytes := array[8] of byte;
    fd := sys->open("reference.bin", Sys->OREAD);
    if (fd == nil || sys->read(fd, bytes, len bytes) != len bytes) {
        raise "missing reference fixture";
    }
    messages := chan[2] of array of byte;
    messages <-= bytes;
    first := refs->accept("/rpc", <-messages);
    messages <-= first.payload();
    second := refs->accept("/rpc", <-messages);
    if (first.read() != 41) {
        raise "initial reference value differs";
    }
    first.write(-32);
    observed := second.read();
    if (observed != -32) {
        raise sys->sprint("forwarded reference observed %d", observed);
    }
    sys->print("PASS Limbo live Int32 reference\n");
}
