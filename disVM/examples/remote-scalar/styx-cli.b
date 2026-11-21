# Copyright (c) 2026 bhgv
# EN: Load a library from the bidirectional Styx command-line mount.
# RU: Загрузить библиотеку из двустороннего маунта Styx командной строки.
implement Client;
include "sys.m";
include "processctl.m";
include "mixed.m";

Client: module { init: fn(); };
Wrong: module { wide: fn(value: int): int; };
WrongTuple: module { pair: fn(value: (big,int)): (int,big); };


# EN: Verify that the same library selects the spawned caller.
# RU: Проверить выбор потомка при вызове той же библиотеки.
child(service: Mixed, ctl: Processctl, start: chan of int, done: chan of int)
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
    service := load Mixed "/peer/mounted/compute.dis";
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
    bad := load Wrong "/peer/mounted/compute.dis";
    if (bad != nil ||
        service->wide(big 9223372036854775807) != big 9223372036854775807 ||
        service->raw(big 4294967296) != big 4294967296 ||
        service->wide(-big 9223372036854775807 - big 1) != -big 9223372036854775807 - big 1) {
        raise "wide Styx import or signed result mismatch";
    }
    if ((load WrongTuple "/peer/mounted/compute.dis") != nil) {
        raise "wrong tuple layout accepted";
    }
    (narrow, wide) := service->pair((-2147483647, big 9223372036854775807));
    if (narrow != -2147483646 || wide != big 9223372036854775807) {
        raise "remote tuple lost signed fields";
    }
    {
        service->pair((0, big 0));
        raise "tuple failure did not cross Styx";
    } exception {
        "remote scalar owner error" =>
            ;
    }
    (narrow, wide) = service->pair((1, -big 9223372036854775807 - big 1));
    if (narrow != 2 || wide != -big 9223372036854775807 - big 1) {
        raise "remote tuple did not recover after exception";
    }
    sys->print("PASS bidirectional Styx CLI mount\n");
}
