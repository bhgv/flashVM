# Copyright (c) 2026 bhgv
# EN: Packet cycle retirement with two permanently admitted native peers.
# RU: Освобождение packet cycle owners при двух постоянно допущенных соседях.
implement PacketCycle;
include "sys.m";
PacketCycle: module { init: fn(); };
sys: Sys; # Immutable native module shared before spawning peers.


# EN: Announce readiness, then hold the real sleep frame in the host relay.
# ready/done are rooted channel arguments; no ownership escapes the frame.
# RU: Сообщить о готовности и задержать настоящий sleep-кадр в host-эстафете.
# ready/done — укоренённые каналы; владение остаётся в кадре.
peer(ready, done: chan of int)
{
    ready <-= 1;
    if (sys->sleep(0) != 0) {
        raise "packet cycle peer sleep changed";
    }
    done <-= 1;
}


# EN: Keep two residents alive while the host retires cyclic records.
# The real werrstr call tests production allocation, roots, RC and Fast workers.
# RU: Сохранить двух соседей, пока host освобождает циклические записи.
# Настоящий werrstr проверяет аллокатор, корни, RC и потоки Fast.
init()
{
    sys = load Sys Sys->PATH;
    ready := chan of int;
    done := chan of int;
    spawn peer(ready, done);
    spawn peer(ready, done);
    <-ready;
    <-ready;
    sys->werrstr("packet cycle checkpoint");
    if (<-done != 1 || <-done != 1) {
        raise "packet cycle peers did not resume";
    }
    sys->print("PASS resident packet cycle peers=2\n");
}
