# Copyright (c) 2026 bhgv
# EN: Load a library from the bidirectional Styx command-line mount.
# RU: Загрузить библиотеку из двустороннего маунта Styx командной строки.
implement Client;
include "sys.m";
include "processctl.m";
include "compute.m";

Client: module { init: fn(); };


# EN: Verify that the same library selects the spawned caller.
# RU: Проверить выбор потомка при вызове той же библиотеки.
child(service: Compute, ctl: Processctl, start: chan of int, done: chan of int)
{
    <-start;
    pid := ctl->pid();
    done <-= service->compute(pid);
}


# EN: Check both callers on a single mounted library.
# RU: Проверить двух вызывающих для одной смонтированной библиотеки.
init()
{
    sys := load Sys Sys->PATH;
    ctl := load Processctl Processctl->PATH;
    service := load Compute "/peer/mounted/compute.dis";
    if (ctl == nil || service == nil) {
        raise sys->sprint("could not load mounted library: %r");
    }
    start := chan of int;
    done := chan of int;
    spawn child(service, ctl, start, done);
    spawn child(service, ctl, start, done);
    start <-= 1;
    start <-= 1;
    first := <-done;
    second := <-done;
    if (first <= 0 || second <= 0 || first == second) {
        raise "provider did not see both concurrent callers";
    }
    sys->print("PASS bidirectional Styx CLI mount\n");
}
