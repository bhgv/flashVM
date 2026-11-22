# Copyright (c) 2026 bhgv
# EN: Repeat launches through the standalone Styx server.
# RU: Повторные запуски через самостоятельный сервер Styx.
implement Persistentcli;

include "sys.m";
include "vfsnative.m";
include "processctl.m";
include "vfsexport.m";

Persistentcli: module { init: fn(); };

init()
{
    sys := load Sys Sys->PATH;
    run_shared(sys);
    run_named(sys);
    check_cancel_launch(sys);
    sys->print("PASS standalone persistent Styx launch\n");
}

include "styx-launch-client.b";

# EN: Cancel a live launch while another child completes on the same session.
# RU: Отменить активный запуск, пока другой потомок завершает работу на той же сессии.
check_cancel_launch(sys: Sys)
{
    vfs := load Vfsnative Vfsnative->PATH;
    control := load Processctl Processctl->PATH;
    exports := load Vfsexport Vfsexport->PATH;
    grant := exports->record(("launch-ready", exports->intvar(0)) :: nil);
    if (grant == nil || exports->mount(grant, "/.vfs", 0, 0) != 0)
        raise "could not publish cancellation marker";
    cancel_child_launch(sys, vfs, control, "park");
    reset := sys->open("/.vfs/launch-ready", Sys->OWRITE);
    cleared := array[] of {byte 0, byte 0, byte 0, byte 0};
    if (reset == nil || sys->write(reset, cleared, len cleared) != len cleared)
        raise "could not reset cancellation marker";
    cancel_child_launch(sys, vfs, control, "spin");
}

cancel_child_launch(sys: Sys, vfs: Vfsnative, control: Processctl, mode: string)
{
    started := chan of int;
    done := chan of int;
    spawn park_launch(vfs, control, mode, started, done);
    caller := <-started;
    signal := array[4] of byte;
    for (i := 0; i < 300; i++) {
        ready := sys->open("/.vfs/launch-ready", Sys->OREAD);
        if (ready != nil && sys->read(ready, signal, len signal) == len signal && signal[3] == byte 1)
            break;
        sys->sleep(10);
    }
    if (sys->open("/transport/programs/second.dis", Sys->OREAD) != nil &&
        vfs->runargs("/transport/programs/second.dis", array of byte "secondary") != 1) {
        raise "second image failed during another active launch";
    }
    if (signal[3] != byte 1 || vfs->run("/shared-launch.dis") != 1 ||
        control->cancelwait(caller) != Processctl->Cancelled || <-done != -1 ||
        vfs->run("/shared-launch.dis") != 1) {
        raise "cancelling a live launch disturbed the session";
    }
}

park_launch(vfs: Vfsnative, control: Processctl, mode: string, started: chan of int, done: chan of int)
{
    started <-= control->pid();
    done <-= vfs->runargs("/shared-launch.dis", array of byte mode);
}
