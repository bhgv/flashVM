# Copyright (c) 2026 bhgv
# EN: Publish one live Limbo-created cell for the native Styx launcher.
# RU: Опубликовать живую ячейку Limbo для нативного лаунчера Styx.
implement Example;
include "sys.m";
include "vfsexport.m";
include "processctl.m";
Example: module { init: fn(); };


# EN/RU: Update the owner value and wait for the remote writer's update.
# Изменить значение владельца и дождаться изменения от удалённого клиента.
init()
{
    sys := load Sys Sys->PATH;
    exports := load Vfsexport Vfsexport->PATH;
    process := load Processctl Processctl->PATH;
    if (process == nil || process->pid() <= 0) {
        raise "could not read client process identity";
    }
    target := "/cell." + string process->pid();
    cell := exports->intvar(41);
    if (cell == nil || exports->mount(cell, target, 0, 0) != 0 || !exports->intset(cell, -2)) {
        raise "could not publish live integer";
    }
    for (attempt := 0; attempt < 400; attempt++) {
        (ok, value) := exports->intget(cell);
        if (!ok) {
            raise "lost exact integer capability";
        }
        if (value == 65539) {
            sys->print("PASS Limbo variable round trip over Styx\n");
            return;
        }
        sys->sleep(10);
    }
    raise "remote writer did not update Limbo value";
}
