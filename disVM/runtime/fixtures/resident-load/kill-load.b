# Copyright (c) 2026 bhgv
# EN: Retire a resident held inside real module open while a peer loader advances.
# RU: Завершить поток внутри настоящего открытия модуля при продвижении другого load.
implement KillLoad;
include "sys.m";
include "processctl.m";
include "provider.m";
KillLoad: module { init: fn(); };
sys: Sys; # Immutable interface initialized before spawn.
control: Processctl; # Exact parent-scoped termination and completion replies.
Warm: con 0; # Driver also preloads the victim's exact image then drops its instance.
Rounds: con 32; # Fresh victim PID and module path in each native open phase.


# EN: Publish PID then load path. reports must remain empty after accepted kill.
# All arguments and the loader continuation belong to this permanent resident.
# RU: Сообщить PID и загрузить path. После принятого kill канал reports пуст.
# Все аргументы и продолжение загрузчика принадлежат закреплённому потоку.
victim(path: string, ids, reports: chan of int)
{
    ids <-= control->pid();
    loaded := load Burst path;
    if (loaded == nil || loaded->run(10) != 37) {
        raise "killed loader executed its post-load code";
    }
    reports <-= 1;
}


# EN: Hold no registry lock across victim I/O. Drop the warm instance, load/call
# a peer before accepting killwait, then verify exact retirement and no continuation.
# RU: Не держать реестр во время I/O жертвы. Снять тёплый экземпляр, загрузить/
# вызвать соседний модуль до killwait и проверить завершение без продолжения.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    ids := chan of int;
    reports := chan[1] of int;
    input := sys->fildes(0);
    permit := array[1] of byte;
    for (round := 0; round < Rounds; round++) {
        path := "slow-" + string round + ".dis";
        anchor: Burst;
        anchor = nil;
        if (Warm) {
            anchor = load Burst path;
            if (anchor == nil || anchor->run(round) != round * 3 + 7) {
                raise "warm loader anchor construction failed";
            }
        }
        spawn victim(path, ids, reports);
        target := <-ids;
        if (sys->read(input, permit, 1) != 1 || permit[0] != byte 49) {
            raise "held loader open was not confirmed";
        }
        anchor = nil;
        peer := load Burst "fast-provider.dis";
        if (peer == nil || peer->run(round + 1000) != (round + 1000) * 3 + 7) {
            raise "held loader prevented unrelated module progress";
        }
        peer = nil;
        sys->print("PASS loader peer phase %d\n", round + 1);
        if (control->killwait(target) != Processctl->Retired ||
                control->killwait(target) != Processctl->Denied ||
                control->cancelwait(target) != Processctl->Denied) {
            raise "held loader did not completely retire";
        }
        alt {
            <-reports => raise "loader executed after accepted termination";
            * => {}
        }
    }
    sys->print("PASS loader kill warm=%d victims=32\n", Warm);
}
