# Copyright (c) 2026 bhgv
# EN: Serve the real DisVM tree and verify effects after the movie cancels acknowledgement waits.
# RU: Обслужить настоящее дерево DisVM и проверить изменения после отмены ожиданий в фильме.
implement Client;
include "sys.m";

Client: module { init: fn(); };
sys: Sys; # Guest's ordinary local namespace and monotonic clock.
BudgetMs: con 9000; # Total budget for all five deterministic holds.
PollMs: con 5; # Interval for the final movie marker.

include "named_movie_upload.b";


# EN: Run the movie and inspect the actual local filesystem after all original requests.
# RU: Запустить фильм и проверить местную файловую систему после исходных запросов.
init()
{
    sys = load Sys Sys->PATH;
    start("cancel-vfs-control.swf");
    began := sys->millisec();
    while (sys->millisec()-began < BudgetMs) {
        (done, nil) := sys->stat("/control-done");
        if (done == 0) {
            (made, info) := sys->stat("/created-by-cancel");
            (removed, nil) := sys->stat("/remove-by-cancel");
            if (made != 0 || info.length != big 0 || removed == 0) {
                raise "unacknowledged mutations did not take effect exactly once";
            }
            sys->print("PASS native cancel-vfs-control RPC paired VMs\n");
            return;
        }
        sys->sleep(PollMs);
    }
    raise "missing cancelled VFS control completion marker";
}
