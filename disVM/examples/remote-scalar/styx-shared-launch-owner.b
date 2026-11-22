# Copyright (c) 2026 bhgv
# EN: Exercise the configured application entry on the combined Styx server.
# RU: Проверить настроенную точку входа программы на общем сервере Styx.
implement Sharedlaunch;
include "sys.m";
include "launchcontext.m";

Sharedlaunch: module { init: fn(); };


# EN: Verify the private context and record execution in the provider's VFS.
# RU: Проверить частный контекст и отметить исполнение в VFS провайдера.
init()
{
    sys := load Sys Sys->PATH;
    pid := sys->open("/.prog/.self/.pid", Sys->OREAD);
    data := array[32] of byte;
    if (pid == nil || sys->read(pid, data, len data) <= 0) {
        raise "combined launch has no caller PID";
    }
    context := load Launchcontext Launchcontext->PATH;
    if (context == nil) {
        raise "combined launch has no startup context";
    }
    case string context->arguments() {
        "wait" =>
            mark(sys, "/parallel-entered");
            wait_for(sys, "/parallel-released");
            return;
        "release" =>
            wait_for(sys, "/parallel-entered");
            mark(sys, "/parallel-released");
            raise "expected failure of the parallel release launch";
    }
    output := sys->open("/launch-marker", Sys->OWRITE);
    if (output == nil || sys->fprint(output, "ran\n") != 4) {
        raise "could not record combined launch";
    }
    if (sys->open("/launch-fail", Sys->OREAD) != nil) {
        raise "requested combined launch failure";
    }
}

# EN: Coordinate two owner processes without involving the caller's namespace.
# RU: Согласовать процессы владельца внутри его пространства имён.
mark(sys: Sys, path: string)
{
    fd := sys->open(path, Sys->OWRITE);
    if (fd == nil || sys->fprint(fd, "1") != 1) {
        raise "could not create launch barrier";
    }
}

wait_for(sys: Sys, path: string)
{
    bytebuf := array[1] of byte;
    for (i := 0; i < 300; i++) {
        fd := sys->open(path, Sys->OREAD);
        if (fd != nil && sys->read(fd, bytebuf, 1) == 1 && bytebuf[0] == byte '1')
            return;
        sys->sleep(10);
    }
    raise "parallel launch did not reach the barrier";
}
