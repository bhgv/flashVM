# Copyright (c) 2026 bhgv
# EN: Call one loaded Styx module from two Limbo processes.
# RU: Вызвать один загруженный модуль Styx из двух процессов Limbo.
implement Client;
include "sys.m";
include "processctl.m";
include "compute.m";

Client: module { init: fn(); };

child(service: Compute, ctl: Processctl, done: chan of int)
{
    pid := ctl->pid();
    result := service->compute(pid);
    if (result != pid)
        result = -1;
    done <-= result;
}

init()
{
    sys := load Sys Sys->PATH;
    ctl := load Processctl Processctl->PATH;
    service := load Compute "/peer/mounted/compute.dis";
    if (ctl == nil || service == nil)
        raise sys->sprint("could not load test modules: %r");
    pid := ctl->pid();
    # EN: The integer argument is only the test oracle.
    # RU: Аргумент-целое нужен только для сверки в тесте.
    if (service->compute(pid) != pid)
        raise "parent caller PID mismatch";
    done := chan of int;
    spawn child(service, ctl, done);
    result := <-done;
    if (result <= 0 || result == pid)
        raise "child caller PID mismatch";
    fd := sys->fildes(3);
    tail := array[4] of byte;
    if (fd == nil || sys->read(fd, tail, len tail) != len tail || string tail != "last")
        raise "remote reads did not advance the local FD";
    if (service->compute(-pid) != pid)
        raise "could not acknowledge the local FD read";
}
