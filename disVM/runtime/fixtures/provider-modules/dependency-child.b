# Copyright (c) 2026 bhgv
# EN: A child spawned from an unbound dependency retains its invoking instance's context without caller frames.
# RU: Child из unbound dependency удерживает context вызывающего instance без caller frames.
implement Value;
include "value.m";
include "sys.m";
include "launchcontext.m";


# EN/RU: A separate dedicated process reads inherited instance metadata and namespace, then sends one result on done.
# Native VM shutdown joins retirement; done stays rooted by the parent through the unbuffered exchange.
# Отдельный dedicated process читает inherited metadata/VFS, затем посылает result в rooted done.
child(done: chan of int)
{
    launch := load Launchcontext Launchcontext->PATH;
    sys := load Sys Sys->PATH;
    if (launch == nil || launch->name() != "/app.dis") {
        raise "dependency child lost instance label";
    }
    arguments := launch->arguments();
    if (len arguments != 3 || int arguments[0] != 77 || int arguments[1] != 0 || int arguments[2] != 1) {
        raise "dependency child inherited caller process context";
    }
    data := sys->open("/data", Sys->OREAD);
    bytes := array[32] of byte;
    n := sys->read(data, bytes, len bytes);
    if (n != 9 || string bytes[:n] != "execution") {
        raise "dependency child lost invoking module namespace";
    }
    done <-= EXPECTED_VALUE;
}


# EN/RU: Spawn a real child from this unbound module and return its channel result; no synthetic context result.
# Создать настоящий child из unbound module и вернуть channel result, без synthetic context result.
value(): int
{
    done := chan of int;
    spawn child(done);
    return <-done;
}
