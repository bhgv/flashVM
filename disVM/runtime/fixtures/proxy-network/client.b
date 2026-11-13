# Copyright (c) 2026 bhgv
# EN: Concurrent remote variable/function images in another real DisVM process.
# RU: Конкурентные remote variable/function images в другом настоящем DisVM process.
implement ProxyNetworkClient;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
ProxyNetworkClient: module { init: fn(); };
net: Proxynet; # Immutable client broker module after init.


# EN/RU: Start the original broker child, publishing its PID before connection IO.
# Запустить original broker child, передав PID до connection IO.
connect(input, output: ref Sys->FD, started, stopped: chan of int,
        ready: chan of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image))
{
    control := load Processctl Processctl->PATH;
    started <-= control->pid();
    net->start(input, output, big 12345, big 67890, 5000, ready);
    stopped <-= 1;
}


# EN/RU: Call one function image repeatedly with two aliases to the same remote variable.
# Вызывать function image с двумя aliases одной удалённой переменной.
client(endpoint: ref Proxynet->Endpoint, variable, function: Proxynet->Image, done: chan of int)
{
    for (round := 0; round < 2; round++) {
        (clone_status, copy) := net->clone(endpoint, variable, 10000);
        if (clone_status != 0) {
            raise "concurrent network image clone";
        }
        (status, value) := net->call(endpoint, Proxyowner->Invoke, function, copy, copy, 2, 1, 10000);
        if (status != 0 || value < 12) {
            raise "network concurrent original ref invocation";
        }
        alias := copy;
        (status, value) = net->call(endpoint, Proxyowner->Drop, copy, copy, copy, 0, 0, 10000);
        if (status != 0) {
            raise "concurrent network image release";
        }
        (status, value) = net->call(endpoint, Proxyowner->Read, alias, alias, alias, 0, 0, 10000);
        if (status != Proxynet->Stale) {
            raise "same-image alias released remote ownership twice";
        }
    }
    done <-= 1;
}


# EN/RU: Exercise scalar mutation, real callback/error, epoch refusal and explicit lifecycle.
# Проверить scalar mutation, настоящий callback/error, epoch refusal и explicit lifecycle.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    net = load Proxynet Proxynet->PATH;
    if (net == nil) {
        raise "network client module";
    }
    net->init();
    started := chan[1] of int;
    stopped := chan[1] of int;
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    spawn connect(sys->fildes(0), sys->fildes(1), started, stopped, ready);
    broker := <-started;
    (connected, endpoint, variable, function) := <-ready;
    if (connected != 0) {
        raise "network fixture greeting refused";
    }
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    (status, value) := net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 10) {
        raise "network original variable read";
    }
    done := chan of int;
    for (index := 0; index < 8; index++) {
        spawn client(endpoint, variable, function, done);
    }
    for (index = 0; index < 8; index++) {
        <-done;
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 42) {
        raise "network ref aliases lost updates";
    }
    forged := variable;
    forged.owner = function.owner;
    (status, value) = net->call(endpoint, Proxyowner->Drop, forged, empty, empty, 0, 0, 10000);
    if (status != Proxynet->Stale) {
        raise "network image owner did not match its native lease";
    }
    (clone_status, retained) := net->clone(endpoint, variable, 10000);
    if (clone_status != 0 || retained.lease == variable.lease) {
        raise "network clone did not create distinct owner";
    }
    old := variable;
    (status, value) = net->call(endpoint, Proxyowner->Drop, old, empty, empty, 0, 0, 10000);
    if (status != 0) {
        raise "network original image release";
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, old, empty, empty, 0, 0, 10000);
    if (status != Proxynet->Stale) {
        raise "network released image was reused";
    }
    variable = retained;
    retained = empty;
    (status, value) = net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 42) {
        raise "network retained clone lost remote owner";
    }
    garbage: Proxynet->Image;
    (clone_status, garbage) = net->clone(endpoint, variable, 10000);
    if (clone_status != 0) {
        raise "network GC clone failed";
    }
    garbage = empty;
    collected := 0;
    for (attempt := 0; collected < 1 && attempt < 1000; attempt++) {
        (status, collected) = net->collected(endpoint, 10000);
        sys->sleep(1);
    }
    if (collected < 1) {
        raise "network image GC did not release remote ownership";
    }
    stale := variable;
    stale.low++;
    (status, value) = net->call(endpoint, Proxyowner->Write, stale, empty, empty, 0, 999, 10000);
    if (status != Proxynet->Stale) {
        raise "network stale incarnation admitted";
    }
    (status, value) = net->call(endpoint, Proxyowner->Write, variable, empty, empty, 0, 100, 10000);
    if (status != 0 || value != 100) {
        raise "network original variable write";
    }
    (status, value) = net->call(endpoint, Proxyowner->Invoke, function, variable, variable, 2, -1, 10000);
    if (status != Proxyowner->OwnerError) {
        raise "network callback exception outcome";
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    if (status != 0 || value != 99) {
        raise "network partial callback effect";
    }
    (status, value) = net->call(endpoint, Proxyowner->Drop, variable, empty, empty, 0, 0, 10000);
    if (status != 0) {
        raise "network drop";
    }
    (status, value) = net->call(endpoint, Proxyowner->Read, variable, empty, empty, 0, 0, 10000);
    if (status == 0) {
        raise "network dropped image still resolves";
    }
    (status, value) = net->call(endpoint, Proxyowner->Stop, empty, empty, empty, 0, 0, 10000);
    if (status != 0 || <-stopped != 1) {
        raise "network broker stop/join";
    }
    control->killwait(broker);
    sys->fprint(sys->fildes(2), "PASS network client: 8 clients actual ref aliases epochs callbacks drop join\n");
}
