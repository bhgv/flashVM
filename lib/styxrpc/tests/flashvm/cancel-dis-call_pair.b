# Copyright (c) 2026 bhgv
# EN: Gate cancellation on remote execution and check prompt unwind on both sides.
# RU: Разрешить отмену после входа в удалённый вызов и проверить быстрый выход с обеих сторон.
implement Client;
include "sys.m";

Client: module { init: fn(); };
sys: Sys; # Supplies the paired test's clock and namespace markers.
ArrivalPolls: con 1000; # Bound waiting for the first marker.
PollMs: con 5; # Sleep interval while waiting for a marker.
StopBudgetMs: con 600; # Shorter than the stock 1000 ms reply deadline.
ParkMs: con 50; # Give the owner a turn to publish its Limbo channel wait.

include "named_movie_upload.b";


# EN: Return when path appears before the supplied monotonic deadline.
# RU: Вернуться, когда path появился до указанного монотонного срока.
wait_for(path: string, started, budget: int)
{
    while (sys->millisec()-started < budget) {
        (ok, nil) := sys->stat(path);
        if (ok == 0) {
            return;
        }
        sys->sleep(PollMs);
    }
    raise "missing scoped call marker " + path;
}


# EN: Keep the pair live until the cancelled owner unwinds and a second proxy succeeds.
# RU: Удерживать пару до выхода отменённого владельца и успеха вызова другого прокси.
init()
{
    sys = load Sys Sys->PATH;
    start("cancel-dis-call.swf");
    arrival := sys->millisec();
    wait_for("/channel-entered", arrival, ArrivalPolls*PollMs);
    wait_for("/text-entered", arrival, ArrivalPolls*PollMs);
    wait_for("/graph-entered", arrival, ArrivalPolls*PollMs);
    # Give the admitted call time to register its local channel wait.
    sys->sleep(ParkMs);
    gate := sys->open("/call-gate", Sys->OWRITE);
    signal := array of byte "1";
    started := sys->millisec();
    if (gate == nil || sys->write(gate, signal, len signal) != len signal) {
        raise "cannot signal scoped call cancellation";
    }
    wait_for("/channel-unwound", started, StopBudgetMs);
    wait_for("/text-unwound", started, StopBudgetMs);
    wait_for("/graph-unwound", started, StopBudgetMs);
    wait_for("/call-finished", started, StopBudgetMs);
    sys->print("PASS native cancel-dis-call RPC paired VMs\n");
}
