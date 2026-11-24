# Copyright (c) 2026 bhgv
# EN: Exercise native $Unibus on ordinary typed Limbo channels against an uploaded SWF.
# RU: Проверить нативный $Unibus на обычных типизированных каналах Limbo и загруженном SWF.
implement Client;
include "sys.m";
include "unibus.m";

Client: module { init: fn(); };


# EN: Upload the counted fixture and attach typed request and reply channels.
# RU: Загрузить SWF с длиной и подключить типизированные каналы запросов и ответов.
init()
{
    sys := load Sys Sys->PATH;
    bus := load Unibus Unibus->PATH;
    input := chan of array of byte;
    output := chan of array of byte;
    source := sys->open("terminal.swf", Sys->OREAD);
    staging := array[4096] of byte;
    size := sys->read(source, staging[4:], len staging - 4);
    if (size <= 0 || size >= len staging - 4) {
        raise "could not read terminal movie";
    }
    staging[2] = byte (size >> 8);
    staging[3] = byte size;
    upload := sys->open("/rpc/.upload", Sys->OWRITE);
    if (upload == nil || sys->write(upload, staging, size + 4) != size + 4) {
        raise sys->sprint("terminal upload: %r");
    }
    if (bus == nil || bus->attach(input, output) != 0 || bus->attach(input, output) != -1) {
        raise "terminal registration was not exclusive";
    }
    state := sys->open("/rpc/.state", Sys->OWRITE);
    if (state == nil || sys->write(state, array of byte "start", 5) != 5) {
        raise sys->sprint("terminal start: %r");
    }
    event := <-input;
    if (string event != "fvd1:000000040000000000000041000000010000004100000000") {
        raise "SWF key frame did not reach Limbo input";
    }
    output <-= array of byte "fvd1:000000020000002a00000000000000000000004200000000Я";
    confirmed := <-input;
    if (string confirmed != "fvd1:000000070000000000000000000000000000004200000000") {
        raise "SWF did not acknowledge the typed reply";
    }
    bus->detach();
    sys->sleep(40); # Keep the connection until the presentation owner has drawn.
    sys->print("PASS paired Unibus channels\n");
}
