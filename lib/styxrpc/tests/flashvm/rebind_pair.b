# Copyright (c) 2026 bhgv
# EN: Change the shared namespace from another Limbo thread after the peer attaches.
# RU: Менять общее пространство имён из другого потока Limbo после подключения участника.
implement Client;
include "sys.m";
include "styxexec.m";

Client: module { init: fn(); };
sys: Sys; # Runtime filesystem and scheduler.
exec: Styxexec; # Phase control over the existing paired mount.
Target: con "/live"; # Virtual mount point already containing the base directory.
Front: con "/front"; # First-priority or replacement directory.
Tail: con "/tail"; # Fallback directory with one unique name.

include "named_movie_upload.b";


# EN: Run all namespace changes on one worker that shares the original namespace.
# RU: Выполнить все изменения на одном рабочем потоке, разделяющем исходное пространство имён.
edit(commands, results: chan of int)
{
    # Absolute names deliberately ignore the changed group cwd.
    # Абсолютные имена намеренно игнорируют изменённый cwd группы.
    for (;;) {
        phase := <-commands;
        status := -1;
        case phase {
        1 =>
            status = sys->chdir(Front);
            if (status == 0) {
                status = sys->bind(Tail, Target, Sys->MAFTER);
            }
        2 =>
            status = sys->bind(Front, Target, Sys->MBEFORE);
        3 =>
            status = sys->unmount(Front, Target);
        4 =>
            status = sys->bind(Front, Target, Sys->MREPL);
        5 =>
            status = sys->unmount(nil, Target);
        * =>
            return;
        }
        results <-= status;
    }
}


# EN: Wait for one positive acknowledgement published by the source-built movie.
# RU: Дождаться положительного подтверждения от собранного из исходника фильма.
await(path: string)
{
    for (i := 0; i < 160; ++i) {
        (status, info) := sys->stat(path);
        if (status == 0) {
            return;
        }
        sys->sleep(10);
    }
    raise "FlashVM did not confirm namespace state: " + path;
}


# EN: Trigger a worker edit, check its result and ask the peer to observe the live namespace.
# RU: Запустить изменение в рабочем потоке, проверить результат и запросить наблюдение у участника.
step(commands, results: chan of int, phase, current, names: int)
{
    commands <-= phase;
    if (<-results < 0) {
        raise sys->sprint("namespace change %d failed", phase);
    }
    payload := array[] of {byte phase, byte current, byte names};
    if (len exec->call("/peer", "rebind-signal", payload)) {
        raise "unexpected rebind callback data";
    }
    await(sys->sprint("/peer/rebind-acks/done%d", phase));
}


# EN: Keep one attached session for the suffix, prefix, removal, replacement and restoration.
# RU: Использовать одну сессию для добавления в конец и начало, снятия, замены и восстановления.
init()
{
    sys = load Sys Sys->PATH;
    exec = load Styxexec Styxexec->PATH;
    if (exec == nil) {
        raise "cannot load Styxexec";
    }
    exec->init();
    commands := chan of int;
    results := chan of int;
    spawn edit(commands, results);
    start("rebind.swf");
    await("/peer/rebind-signal");
    step(commands, results, 1, 65, 3);
    step(commands, results, 2, 67, 7);
    step(commands, results, 3, 65, 3);
    step(commands, results, 4, 67, 4);
    step(commands, results, 5, 65, 1);
    commands <-= 0;
    sys->print("PASS native rebind RPC paired VMs\n");
}
