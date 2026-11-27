# Copyright (c) 2026 bhgv
# EN: Exchange through a native AS task, cancel unmatched operations and disconnect.
# RU: Обмен через системную задачу AS, отмена операций без пары и отключение.
implement Client;
include "sys.m";
include "styxnamed.m";

Client: module { init: fn(); };
sys: Sys; # System services of the mounted Limbo program.
named: Styxnamed; # Typed channel access for the paired registry.
Port: import named;

include "named_movie_upload.b";


# EN: Wait for the namespace observation to become true, without opening a channel.
# RU: Дождаться нужного состояния имени без открытия канала.
await(path: string, exists: int)
{
    for (i := 0; i < 160; ++i) {
        (ok, info) := sys->stat(path);
        if ((ok == 0) == exists) {
            return;
        }
        sys->sleep(10);
    }
    raise "unexpected channel namespace state: " + path;
}


# EN: Rendezvous on one channel while two unmatched worker operations remain pending.
# RU: Обменяться в одном канале, пока две операции потока ждут встречного участника.
init()
{
    sys = load Sys Sys->PATH;
    named = load Styxnamed Styxnamed->PATH;
    if (named == nil) {
        raise "cannot load Styxnamed";
    }
    named->init();
    let := named->open("/work");
    start("asinc-chan.swf");
    await("/wait-read", 1);
    await("/wait-write", 1);
    frame := array[] of {byte Styxnamed->Text, byte 't', byte 'y', byte 'p', byte 'e', byte 'd'};
    let.send(frame);
    reply := let.recv();
    if (len reply != len frame || reply[0] != byte Styxnamed->Text || string reply[1:] != "typed") {
        raise "wrong worker channel echo";
    }
    let.close();
    await("/ack", 1);
    await("/wait-read", 0);
    await("/wait-write", 0);
    await("/work", 0);
    await("/abandoned", 1);
    sys->print("PASS native asinc-chan RPC paired VMs\n");
}
