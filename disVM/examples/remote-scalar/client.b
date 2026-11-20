# Copyright (c) 2026 bhgv
# EN: Scalar caller of the generated owner across an existing Proxynet connection.
# RU: Scalar caller generated owner через существующее соединение Proxynet.
implement RemoteScalarClient;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "compute-remote.m";
RemoteScalarClient: module { init: fn(); };
net: Proxynet; # Initialized before spawn / Инициализируется до spawn.


# EN: Own the permanent broker Prog while borrowing inherited stream directions.
# Announce PID before I/O and completion after start retires. No reconnect/replay.
# RU: Владеть permanent broker Prog, заимствуя inherited stream directions.
# Сообщить PID до I/O, completion после retirement start. Без reconnect/replay.
connect(input, output: ref Sys->FD, started, stopped: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    control := load Processctl Processctl->PATH;
    started <-= control->pid();
    net->start(input, output, big 12345, big 67890, 5000, ready);
    stopped <-= 1;
}


# EN: Verify remote implementation state, exception and recovery, then stop/join.
# The client has neither implementation.dis nor provider-secret in its namespace.
# RU: Проверить remote state, exception/recovery, затем stop/join. У клиента нет
# implementation.dis и provider-secret в namespace.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    net = load Proxynet Proxynet->PATH;
    if (net == nil) { raise "remote scalar broker unavailable"; }
    net->init();
    started := chan[1] of int;
    stopped := chan[1] of int;
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    spawn connect(sys->fildes(0), sys->fildes(1), started, stopped, ready);
    broker := <-started;
    (connected, endpoint, variable, function) := <-ready;
    if (connected != 0) { raise "remote scalar greeting failed"; }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    remote := load ComputeRemote "compute-remote.dis";
    if (remote == nil) { raise "generated client module unavailable"; }
    unbound := 0;
    {
        remote->compute(10);
    } exception {
        "remote client is unbound" => unbound = 1;
    }
    if (!unbound) { raise "unbound client performed a call"; }
    refused := 0;
    foreign := function;
    foreign.high = big 0;
    {
        remote->remote_bind(endpoint, foreign, 10000);
    } exception {
        "remote client binding refused" => refused = 1;
    }
    if (!refused) { raise "foreign session accepted"; }
    remote->remote_bind(endpoint, function, 10000);
    refused = 0;
    {
        remote->remote_bind(endpoint, function, 10000);
    } exception {
        "remote client binding refused" => refused = 1;
    }
    if (!refused) { raise "live binding replaced"; }
    if (remote->compute(10) != 52 || remote->compute(20) != 63) {
        raise "provider value/state lost";
    }
    caught := 0;
    {
        remote->compute(-1);
    } exception {
        "remote owner error" => caught = 1;
    }
    if (!caught || remote->compute(40) != 85) {
        raise "provider error or recovery lost";
    }
    (status, unused) := net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (status != 0 || <-stopped != 1) { raise "remote scalar stop failed"; }
    control->killwait(broker);
    sys->fprint(sys->fildes(2), "PASS generated scalar owner: provider code resource state error recovery\n");
}
