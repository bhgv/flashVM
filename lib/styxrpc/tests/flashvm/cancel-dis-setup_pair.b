# Copyright (c) 2026 bhgv
# EN: Run the movie on the paired mount until it confirms all setup cancellations.
# RU: Запустить фильм на парном монтировании и дождаться проверки отмены подготовки.
implement Client;
include "sys.m";

Client: module { init: fn(); };
sys: Sys; # Real namespace and monotonic clock for the bounded wait.
BudgetMs: con 9000; # Total budget for the five holds and recovery.
PollMs: con 5; # Interval for the final movie marker.

include "named_movie_upload.b";


# EN: Start the compiled SWF and wait for its successful recovery calls.
# RU: Запустить собранный SWF и дождаться успешных вызовов после отмены.
init()
{
    sys = load Sys Sys->PATH;
    start("cancel-dis-setup.swf");
    began := sys->millisec();
    while (sys->millisec()-began < BudgetMs) {
        (done, nil) := sys->stat("/control-done");
        if (done == 0) {
            sys->print("PASS native cancel-dis-setup RPC paired VMs\n");
            return;
        }
        sys->sleep(PollMs);
    }
    raise "missing module setup recovery marker";
}
