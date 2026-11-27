# Copyright (c) 2026 bhgv
# EN: Use the published AS object image from an ordinary Limbo client.
# RU: Использовать опубликованный образ AS из обычного клиента Limbo.
implement Client;
include "sys.m";

Client: module { init: fn(); };
Peer: module {
    Leaf: adt { value: int; };
    Box: adt { leaf: ref Leaf; };
    magnitude: fn(n: int): int;
    realMagnitude: fn(n: real, label: string): real;
    graphMagnitude: fn(n: real, box: ref Box): real;
};
sys: Sys;


# EN: Stage the compiler-built provider film through the public playback nodes.
# RU: Загрузить собранный фильм провайдера через общедоступные узлы управления.
start()
{
    source := sys->open("peer-provider.swf", Sys->OREAD);
    bytes := array[16384] of byte;
    size := sys->read(source, bytes[4:], len bytes - 4);
    fd := sys->open("/rpc/.upload", Sys->OWRITE);
    if (size <= 0 || size >= len bytes - 4 || fd == nil) {
        raise sys->sprint("cannot stage peer movie: %r");
    }
    bytes[0] = byte (size >> 24);
    bytes[1] = byte (size >> 16);
    bytes[2] = byte (size >> 8);
    bytes[3] = byte size;
    for (offset := 0; offset < size + 4;) {
        sent := sys->write(fd, bytes[offset:size + 4], size + 4 - offset);
        if (sent <= 0) {
            raise sys->sprint("upload peer movie: %r");
        }
        offset += sent;
    }
    state := sys->open("/rpc/.state", Sys->OWRITE);
    data := array of byte "start";
    if (state == nil || sys->write(state, data, len data) != len data) {
        raise sys->sprint("start peer movie: %r");
    }
}


# EN: Read the exact Int32 cell through the mounted Styx service and compare its value.
# RU: Прочитать точную ячейку Int32 через смонтированный сервис Styx и сверить значение.
check(path: string, expected: int)
{
    fd := sys->open(path, Sys->OREAD);
    bytes := array[4] of byte;
    if (fd == nil || sys->read(fd, bytes, len bytes) != len bytes ||
        int bytes[0] != 0 || int bytes[1] != 0 || int bytes[2] != 0 || int bytes[3] != expected) {
        raise sys->sprint("unexpected FlashVM variable: %r");
    }
}


# EN: Call one image export, then confirm live Int32 access through two distinct fids.
# RU: Вызвать экспорт образа и подтвердить живой доступ к Int32 через два разных fid.
init()
{
    sys = load Sys Sys->PATH;
    start();
    path := "/rpc/mounted/peer.dis";
    peer: Peer;
    for (i := 0; i < 60 && peer == nil; ++i) {
        peer = load Peer path;
        if (peer == nil) {
            sys->sleep(20);
        }
    }
    if (peer == nil || peer->magnitude(-12) != 12) {
        raise sys->sprint("FlashVM scalar entry unavailable: %r");
    }
    box := ref Peer->Box(ref Peer->Leaf(43));
    if (peer->realMagnitude(-12.5, "signed") != 12.5 || peer->graphMagnitude(-14.5, box) != 14.5 ||
        box.leaf.value != 43) {
        raise "FlashVM shaped entry lost its result or graph data";
    }
    variable := path + "/.var/count";
    check(variable, 41);
    fd := sys->open(variable, Sys->OWRITE);
    data := array[] of {byte 0, byte 0, byte 0, byte 77};
    if (fd == nil || sys->write(fd, data, len data) != len data) {
        raise sys->sprint("FlashVM variable write failed: %r");
    }
    check(variable, 77);
    sys->print("PASS DisVM called the AS module and updated its live variable\n");
}
