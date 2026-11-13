# Copyright (c) 2026 bhgv
# EN: Native socket output must unblock on peer loss without changing descriptor flags.
# RU: Native socket output обязан разблокироваться при потере peer без смены fd flags.
implement ProxySocketOutput;
include "sys.m";
ProxySocketOutput: module { init: fn(); };


# EN/RU: Fill socket output until peer closure; require an ordinary IO error, never SIGPIPE.
# Заполнять socket output до закрытия peer; ждать обычный IO error, не SIGPIPE.
writer(done: chan of int)
{
    sys := load Sys Sys->PATH;
    output := sys->fildes(1);
    bytes := array[4096] of byte;
    for (index := 0; index < len bytes; index++) {
        bytes[index] = byte 65;
    }
    sys->fprint(sys->fildes(2), "NETWORK socket output entered\n");
    for (round := 0; round < 10000; round++) {
        if (sys->write(output, bytes, len bytes) < 0) {
            done <-= 1;
            return;
        }
    }
    raise "socket output never observed peer loss";
}


# EN/RU: Keep parent alive until the original native writer reports its IO error.
# Сохранить parent до сообщения original native writer об IO error.
init()
{
    sys := load Sys Sys->PATH;
    done := chan[1] of int;
    spawn writer(done);
    if (<-done != 1) {
        raise "socket output child failed";
    }
    sys->fprint(sys->fildes(2), "PASS socket output peer loss returns IO error\n");
}
