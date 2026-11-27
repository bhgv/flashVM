# Copyright (c) 2026 bhgv
# EN: Confirm paired channel setup cancellation and retirement of the native name reservation.
# RU: Проверить отмену подготовки каналов и снятие нативного резервирования имени.
implement Client;
include "sys.m";

Client: module { init: fn(); };
sys: Sys; # Real namespace and monotonic clock for the bounded wait.
BudgetMs: con 9000; # Total budget for the five holds and recovery.
PollMs: con 5; # Poll interval for the movie's recovery marker.

include "named_movie_upload.b";


# EN: Start the paired movie. Verify that neither cancelled enrollment leaves a VFS name.
# RU: Запустить фильм пары и проверить, что отменённое участие не оставило имён VFS.
init()
{
    sys = load Sys Sys->PATH;
    start("cancel-channel-setup.swf");
    began := sys->millisec();
    while (sys->millisec()-began < BudgetMs) {
        (done, nil) := sys->stat("/control-done");
        if (done == 0) {
            (opened, nil) := sys->stat("/cancel-open");
            (enrolled, nil) := sys->stat("/cancel-enroll");
            if (opened == 0 || enrolled == 0) {
                raise "cancelled channel left a name in the shared namespace";
            }
            sys->print("PASS native cancel-channel-setup RPC paired VMs\n");
            return;
        }
        sys->sleep(PollMs);
    }
    raise "missing channel setup recovery marker";
}
