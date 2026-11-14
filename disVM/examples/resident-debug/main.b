# Copyright (c) 2026 bhgv
# EN: Run a real worker and inspect its owner-published module and next Dis PC.
# RU: Запустить worker и прочитать опубликованные owner модуль и next Dis PC.
implement ResidentDebugExample;
include "sys.m";
include "processctl.m";
include "residentdebug.m";
ResidentDebugExample: module { init: fn(); };


# EN/RU: Block on ordinary gate after publishing own PID; parent retires this owner.
# Ждать ordinary gate после передачи PID; parent завершает этого owner.
worker(ready, gate: chan of int)
{
    ctl := load Processctl Processctl->PATH;
    ready <-= ctl->pid();
    <-gate;
}


# EN: Compile library beside this module. Inspect this VM's worker, then join it.
# Display labels identify a capture; no arbitrary memory/debug control is granted.
# RU: Собрать library рядом с модулем. Прочитать worker своей VM, затем join.
# Display labels относятся к capture и не дают memory/debug control.
init()
{
    sys := load Sys Sys->PATH;
    ctl := load Processctl Processctl->PATH;
    debug := load Residentdebug Residentdebug->PATH;
    ready := chan of int;
    gate := chan of int;
    spawn worker(ready, gate);
    pid := <-ready;
    id := big 0;
    cursor := big 0;
    for (;;) {
        (status, found, owner, done) := ctl->resident(cursor);
        if (status != 1) {
            raise "example worker registry missing";
        }
        if (owner == pid) {
            id = found;
            break;
        }
        cursor = found;
    }
    (status, location) := debug->inspect(id, 1000);
    if (ctl->killwait(pid) != ctl->Retired) {
        raise "example worker retirement failed";
    }
    if (status != ctl->DiagReady || location == nil || !(location.flags & ctl->LocationValid)) {
        raise "example owner location unavailable";
    }
    sys->print("module: %s\npath: %s\nnext Dis PC: %bd / %d; flags=%d\n",
               location.name, location.path, location.nextpc, location.instructions, location.flags);
    sys->print("PASS resident debug example\n");
}
