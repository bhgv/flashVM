# Copyright (c) 2026 bhgv
# EN: Actual parent/child typed calls prefer their inherited local side without changing data order.
# RU: Настоящие parent/child typed calls предпочитают inherited local side, не меняя data order.
implement Check;
include "sys.m";
include "value.m";
include "processctl.m";
Check: module
{
    init: fn(); # Ordinary zero-argument startup bootstrap.
};


# EN/RU: Load/call an actual application and dependency on this permanent child's inherited lookup side.
# Загрузить/вызвать настоящее приложение и зависимость на inherited lookup side постоянного child.
child(done: chan of (int, int))
{
    app := load Value "/app.dis";
    if (app == nil) {
        raise "child could not load caller-local application";
    }
    ctl := load Processctl Processctl->PATH;
    done <-= (ctl->pid(), app->value());
    never := chan of int;
    <-never;
}


# EN/RU: Parent and child use the same typed interface and independent source selection; ordinary data stays terminal-first.
# Parent и child используют тот же typed interface и independent source selection; data остаётся terminal-first.
init()
{
    sys := load Sys Sys->PATH;
    ctl := load Processctl Processctl->PATH;
    app := load Value "/app.dis";
    if (app == nil || app->value() != EXPECTED) {
        raise "parent ignored its caller-local module side";
    }
    done := chan of (int, int);
    spawn child(done);
    (pid, result) := <-done;
    if (result != EXPECTED || ctl->killwait(pid) != Processctl->Retired) {
        raise "child lost inherited caller-local module side";
    }
    bytes := array[32] of byte;
    data := sys->open("/data", Sys->OREAD);
    n := sys->read(data, bytes, len bytes);
    if (n != 8 || string bytes[:n] != "terminal") {
        raise "caller-local module lookup changed ordinary data order";
    }
    sys->print("PASS provider-backed Dis module and dependency\n");
}
