# Copyright (c) 2026 bhgv
# EN: Original network owner callback with visible admission and partial effects.
# RU: Исходный network owner callback с наблюдаемым admission и частичными effects.
implement Proxyhandler;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyhandler.m";


# EN/RU: Bind original capture to this serving permanent Prog identity.
# Привязать исходный capture к identity обслуживающего постоянного Prog.
start(capture: ref Proxyhost->Cell)
{
    control := load Processctl Processctl->PATH;
    capture.value = control->pid();
}


# EN: Mutate original alias refs only on the captured owner. -999 publishes entry
# and blocks until connection cleanup kills this owner; -1 raises after one effect.
# RU: Менять исходные alias refs лишь на captured owner. -999 сообщает entry и
# блокируется до kill при cleanup соединения; -1 бросает после одного effect.
invoke(capture, left, right: ref Proxyhost->Cell, value: int): int
{
    control := load Processctl Processctl->PATH;
    sys := load Sys Sys->PATH;
    if (capture.value != control->pid() || left == nil || left != right) {
        raise "network callback lost original owner or aliases";
    }
    if (value == -999) {
        left.value = 777;
        sys->fprint(sys->fildes(2), "NETWORK callback admitted and blocked\n");
        sys->sleep(60000);
        raise "abandoned network callback resumed";
    }
    left.value += value;
    if (value == -1) {
        raise "network partial callback effect";
    }
    right.value += value;
    return left.value;
}
