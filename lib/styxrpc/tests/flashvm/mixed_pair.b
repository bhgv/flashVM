# Copyright (c) 2026 bhgv
# EN: Check native DisVM and FlashVM mixed messages using only Limbo and AS applications.
# RU: Проверка смешанных сообщений штатных DisVM и FlashVM приложениями на Limbo и AS.
implement Client;
include "sys.m";
include "styxintref.m";
include "styxchannel.m";

Client: module { init: fn(); };
sys: Sys; # Ordinary file operations and timer used by the test.
rpc: Styxchannel; # Typed mixed UniBus channels.
Port, Message: import rpc;


# EN: Upload the compiled SWF to the paired player's control endpoint and start it.
# RU: Загрузить собранный SWF в точку управления плеера и запустить его.
start()
{
    fd := sys->open("mixed.swf", Sys->OREAD);
    bytes := array[8192] of byte;
    n := sys->read(fd, bytes[4:], len bytes - 4);
    if (n <= 0 || n >= len bytes - 4) {
        raise "cannot read mixed SWF";
    }
    bytes[2] = byte (n >> 8);
    bytes[3] = byte n;
    upload := sys->open("/peer/.upload", Sys->OWRITE);
    if (upload == nil || sys->write(upload, bytes, n+4) != n+4) {
        raise sys->sprint("cannot upload mixed SWF: %r");
    }
    upload = nil;
    state := sys->open("/peer/.state", Sys->OWRITE);
    command := array of byte "start";
    if (state == nil || sys->write(state, command, len command) != len command) {
        raise sys->sprint("cannot start mixed SWF: %r");
    }
}


# EN: Send one typed message and poll the same port for its echoed reply.
# RU: Отправить типизированное сообщение и получить ответ на том же порту.
exchange(port: ref Port, value: ref Message): ref Message
{
    if (!port.send(value)) {
        raise "mixed test send should fit";
    }
    for (i := 0; i < 300; ++i) {
        reply := port.recv();
        if (reply != nil) {
            return reply;
        }
        sys->sleep(10);
    }
    raise "mixed test reply timed out";
}


# EN: Compare each reply against its original typed payload, including false and null.
# RU: Сравнить ответы с исходными типизированными данными, включая false и null.
check(port: ref Port)
{
    pick a := exchange(port, ref Message.Integer(-2147483647-1)) {
    Integer =>
        if (a.value != -2147483647-1) {
            raise "mixed Int32 differs";
        }
    * => raise "mixed Int32 kind differs";
    }
    pick b := exchange(port, ref Message.Floating(12.25)) {
    Floating =>
        if (b.value != 12.25) {
            raise "mixed real differs";
        }
    * => raise "mixed real kind differs";
    }
    pick c := exchange(port, ref Message.Text("Привет\0world")) {
    Text =>
        if (c.value != "Привет\0world") {
            raise "mixed text differs";
        }
    * => raise "mixed text kind differs";
    }
    pick d := exchange(port, ref Message.Boolean(0)) {
    Boolean =>
        if (d.value) {
            raise "mixed Boolean differs";
        }
    * => raise "mixed Boolean kind differs";
    }
    pick e := exchange(port, ref Message.Null(0)) {
    Null =>
        if (e.reserved) {
            raise "mixed Null differs";
        }
    * => raise "mixed Null kind differs";
    }
}


# EN: Verify the service is invisible through the ordinary mount and then use the RPC API.
# RU: Проверить скрытие службы в обычном маунте и обратиться через API RPC.
init()
{
    sys = load Sys Sys->PATH;
    rpc = load Styxchannel Styxchannel->PATH;
    rpc->init();
    (status, metadata) := sys->stat("/peer/.rpc");
    if (status >= 0 || sys->open("/peer/.rpc", Sys->ORDWR) != nil) {
        raise "mixed port visible in the ordinary VFS";
    }
    dir := sys->open("/peer", Sys->OREAD);
    if (dir == nil) {
        raise "ordinary peer root unavailable";
    }
    (entries, names) := sys->dirread(dir);
    if (entries < 0) {
        raise "ordinary peer listing failed";
    }
    for (i := 0; i < entries; ++i) {
        if (names[i].name == ".rpc") {
            raise "mixed port listed in the ordinary VFS";
        }
    }
    dir = nil;
    start();
    port := rpc->newchan("/.rpc", "/peer");
    refused := 0;
    {
        duplicate := rpc->newchan(".rpc", "/peer");
        duplicate.close();
    } exception {
    "cannot open StyxRPC exchange" => refused = 1;
    }
    if (!refused) {
        raise "mixed mailbox admitted a second peer";
    }
    check(port);
    port.close();
    port.close();
    port = rpc->newchan(".rpc", "/peer");
    check(port);
    port.close();
    sys->print("PASS native mixed RPC paired VMs\n");
}
