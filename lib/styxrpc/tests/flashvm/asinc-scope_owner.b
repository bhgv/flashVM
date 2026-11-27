# Copyright (c) 2026 bhgv
# EN: Keep two concurrent scoped exchanges active while consuming caller-owned channels.
# RU: Удерживать два параллельных контекстных вызова с каналами вызывающего.
implement Scopedworker;

include "sys.m";

Scopedworker: module {
    bounce: fn(input: chan of int): chan of int;
};

Delayms: con 160; # EN: Overlap the argument scopes. RU: Перекрыть по времени области аргументов.


# EN: Read input after a native delay and seed a returned Dis-owned channel.
# RU: Прочитать input после нативной задержки и заполнить канал, принадлежащий Dis.
bounce(input: chan of int): chan of int
{
    sys := load Sys Sys->PATH;
    sys->sleep(Delayms);
    value := <-input;
    output := chan[1] of int;
    output <-= value + 2;
    return output;
}
