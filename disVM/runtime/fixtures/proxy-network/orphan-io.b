# Copyright (c) 2026 bhgv
# EN: Caller retirement must not strand a supervised native socket read.
# RU: Retirement caller не должен оставлять supervised native socket read.
implement ProxyOrphanIo;
include "sys.m";
include "processctl.m";
include "proxystream.m";
ProxyOrphanIo: module { init: fn(); };
stream: Proxystream; # Immutable initialized IO supervisor module.


# EN/RU: Announce own PID and enter bounded IO with no incoming bytes.
# Передать PID и войти в bounded IO без входящих bytes.
reader(input: ref Sys->FD, started: chan of int)
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    started <-= control->pid();
    bytes := array[184] of byte;
    stream->transfer(input, bytes, 0, sys->millisec() + 3000);
    raise "public IO caller survived forced retirement";
}


# EN/RU: Kill only the public caller; private supervisor must later join IO/alarm.
# Уничтожить лишь public caller; private supervisor должен завершить IO/alarm.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    stream = load Proxystream Proxystream->PATH;
    stream->init();
    started := chan[1] of int;
    spawn reader(sys->fildes(0), started);
    caller := <-started;
    sys->sleep(50);
    if (control->killwait(caller) != Processctl->Retired) {
        raise "public IO caller was not retired";
    }
    sys->fprint(sys->fildes(2), "PASS retired IO caller leaves finite supervisor\n");
}
