# Copyright (c) 2026 bhgv
# EN: Coordinate real cancellation of returned-channel operations on the paired player.
# RU: Координировать настоящую отмену операций возвращённых каналов в парном плеере.
implement Client;
include "sys.m";

Client: module { init: fn(); };
sys: Sys; # Supplies the paired test's clock and namespace markers.
StartBudgetMs: con 5000; # Wait for all workers to obtain their exact channel handles.
PollMs: con 5; # Sleep while waiting for a marker from the movie.
StopBudgetMs: con 600; # Shorter than the stock 1000 ms channel deadline.
ParkMs: con 60; # Let the worker write its pending operation before the cancellation gate.

include "named_movie_upload.b";


# EN: Return when path appears before the shared deadline.
# RU: Вернуться, когда path появился до общего срока.
wait_for(path: string, started, budget: int)
{
    while (sys->millisec()-started < budget) {
        (ok, nil) := sys->stat(path);
        if (ok == 0) {
            return;
        }
        sys->sleep(PollMs);
    }
    raise "missing returned channel marker " + path;
}


# EN: Start the pair, let all waits reach the Dis bus, and gate cancellation from the movie.
# RU: Запустить пару, дать ожиданиям дойти до шины Dis и разрешить фильму отмену.
init()
{
    sys = load Sys Sys->PATH;
    start("cancel-dis-channel.swf");
    started := sys->millisec();
    for (i := 0; i < 3; i++) {
        wait_for("/channel-entered-" + string i, started, StartBudgetMs);
    }
    sys->sleep(ParkMs);
    gate := sys->open("/channel-gate", Sys->OWRITE);
    signal := array of byte "1";
    started = sys->millisec();
    if (gate == nil || sys->write(gate, signal, len signal) != len signal) {
        raise "cannot signal cancellation of returned channels";
    }
    wait_for("/channel-finished", started, StopBudgetMs);
    sys->print("PASS native cancel-dis-channel RPC paired VMs\n");
}
