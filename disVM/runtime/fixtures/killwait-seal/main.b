# Copyright (c) 2026 bhgv
# EN: Wait for one naturally retiring child, including sealing during preflight.
# RU: Дождаться естественного завершения ребёнка с sealing во время preflight.
implement KillwaitSeal;
include "sys.m";
include "processctl.m";
KillwaitSeal: module { init: fn(); };
control: Processctl; # Immutable native process interface shared before spawn.


# EN: Publish this child's real PID, then return on the same permanent thread.
# RU: Опубликовать настоящий PID ребёнка и вернуться на постоянном потоке.
child(ready: chan of int)
{
    ready <-= control->pid();
}


# EN: Require the exact retirement acknowledgement, not a missing-child result.
# RU: Потребовать точную квитанцию завершения, а не результат отсутствия ребёнка.
init()
{
    sys := load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    ready := chan of int;
    spawn child(ready);
    pid := <-ready;
    if (control->killwait(pid) != Processctl->Retired) {
        raise "registered child killwait skipped retirement";
    }
    sys->print("PASS killwait admits child sealing during preflight\n");
}
