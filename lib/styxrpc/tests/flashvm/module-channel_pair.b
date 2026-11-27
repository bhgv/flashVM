# Copyright (c) 2026 bhgv
# EN: Call ordinary and channel-valued exports of a stock ActionScript image.
# RU: Вызов обычного и канального экспортов штатного образа ActionScript.
implement Client;
include "sys.m";

Client: module { init: fn(); };
Image: module {
    normalize: fn(n: int): int; # Scalar method on the receiver.
    queue: fn(n: int): chan of int; # A separate live result for each call.
};
sys: Sys; # Borrowed mounted system interface.

include "named_movie_upload.b";


# EN: Load all entries with compiled Limbo signatures, then exercise their shared session.
# RU: Загрузить входы с компилированными сигнатурами Limbo и проверить общую сессию.
init()
{
    sys = load Sys Sys->PATH;
    start("module-channel.swf");
    path := "/peer/mounted/module-channel.dis";
    image: Image;
    for (i := 0; i < 200 && image == nil; ++i) {
        image = load Image path;
        if (image == nil) {
            sys->sleep(10);
        }
    }
    if (image == nil || image->normalize(-12) != 12) {
        raise sys->sprint("cannot call AS module scalar export: %r");
    }
    first := image->queue(-51);
    second := image->queue(-13);
    if (first == nil || second == nil || <-second != 13 || <-first != 51) {
        raise "AS module did not seed independent channels";
    }
    first <-= 88;
    if (<-first != 88) {
        raise "AS module channel did not accept a later send";
    }
    fd := sys->open(path + "/.var/count", Sys->OREAD);
    bytes := array[4] of byte;
    if (fd == nil || sys->read(fd, bytes, len bytes) != len bytes || int bytes[3] != 7) {
        raise "AS module did not expose shared receiver data";
    }
    sys->print("PASS native module-channel RPC paired VMs\n");
}
