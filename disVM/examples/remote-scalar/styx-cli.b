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
child(service: Compute, ctl: Processctl, done: chan of int)
{
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
    pid := ctl->pid();
    if (service->compute(pid) != pid) {
        raise "provider did not see parent context";
    }
    done := chan of int;
    spawn child(service, ctl, done);
    childpid := <-done;
    if (childpid <= 0 || childpid == pid) {
        raise "provider did not see child context";
    }
    sys->print("PASS bidirectional Styx CLI mount\n");
}
