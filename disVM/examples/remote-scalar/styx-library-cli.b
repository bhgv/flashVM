# Copyright (c) 2026 bhgv
# EN: Load two libraries on one ordinary Styx mount and compare their executor IDs.
# RU: Загрузить две библиотеки с одного маунта Styx и сравнить PID исполнителей.
implement Librarycli;

include "sys.m";
include "processctl.m";
include "vfsnative.m";
include "scoped-calls.m";

Librarycli: module
{
    init: fn();
};

# EN: Test ordinary load, scoped ref copy-back and one executor across modules.
# RU: Проверить обычный load, обратную запись ref и общего исполнителя модулей.
init()
{
    sys := load Sys Sys->PATH;
    if (sys->bind("/transport/mounted", "/libraries", Sys->MAFTER) < 0) {
        raise "could not add the Styx library directory to the local union: " + sys->sprint("%r");
    }
    if (sys->open("/libraries/local-marker", Sys->OREAD) == nil) {
        raise "local member of the library union disappeared";
    }
    first := load Scopedcalls "/libraries/first.dis";
    second := load Scopedcalls "/libraries/second.dis";
    if (first == nil || second == nil) {
        raise "could not load both mounted libraries: " + sys->sprint("%r");
    }
    pid := sys->pctl(0, nil);
    a := ref Cell(-51, big 0);
    b := ref Cell(-51, big 0);
    if (first->update(a) != pid || second->update(b) != pid ||
        a.narrow != pid || b.narrow != pid || a.wide <= big 0 || a.wide != b.wide) {
        raise "mounted libraries did not share the expected executor";
    }
    run_shared(sys);
    run_named(sys);
    check_cancel(sys, first, second, a.wide);
    sys->print("PASS two libraries on one stock Styx session\n");
}

include "refpair-sync.b";


# EN: Cancel a child call, then compare replacement and unaffected worker IDs.
# RU: Отменить вызов потомка и сверить PID нового и незатронутого исполнителей.
check_cancel(sys: Sys, first, second: Scopedcalls, parent_worker: big)
{
    control := load Processctl Processctl->PATH;
    started := chan of int;
    done := chan of int;
    spawn interrupted_ref(first, second, control, started, done);
    child := <-started;
    wait_marker(sys, "single-ref-pending");
    cell := ref Cell(-51, big 0);
    if (second->update(cell) != control->pid() || cell.wide != parent_worker) {
        raise "parallel caller lost its executor";
    }
    if (control->cancelwait(child) != Processctl->Cancelled) {
        raise "could not cancel the remote ref wait";
    }
    wait_marker(sys, "single-ref-cancelled");
    if (<-done != 1) {
        raise "cancelled caller did not get a fresh executor on the second library";
    }
}


# EN: Reuse the same caller PID after cancellation and inspect the new Prog.
# RU: Повторно вызвать с тем же PID клиента после отмены и проверить новый Prog.
interrupted_ref(first, second: Scopedcalls, control: Processctl, started, done: chan of int)
{
    pid := control->pid();
    before := ref Cell(-51, big 0);
    valid := first->update(before) == pid;
    started <-= pid;
    pending := ref Cell(-50, big 0);
    cancelled := 0;
    {
        first->update(pending);
    } exception {
        "*" =>
            cancelled = 1;
    }
    after := ref Cell(-51, big 0);
    valid = second->update(after) == pid && valid;
    done <-= valid && cancelled && pending.narrow == -50 && pending.wide == big 0 &&
        before.wide > big 0 && after.wide > big 0 && before.wide != after.wide;
}

include "styx-launch-client.b";
