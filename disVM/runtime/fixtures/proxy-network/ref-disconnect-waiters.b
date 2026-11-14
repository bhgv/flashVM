# Copyright (c) 2026 bhgv
# EN: Eight offered accesses must observe terminal state before their original long deadlines.
# RU: Восемь offered accesses должны увидеть terminal state раньше original long deadlines.
implement DisconnectWaiters;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyroute.m";
DisconnectWaiters: module { init: fn(); };
net: Proxynet; # Immutable native network capability interface.
access: Proxyref; # Immutable variable-only access interface.


# EN/RU: Publish caller PID, offer Add with an unchanged sixty-second deadline, return status.
# Передать PID caller, предложить Add с unchanged sixty-second deadline, вернуть status.
caller(source: ref Proxyref->Ref, started, completed: chan of int)
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    started <-= control->pid();
    (status, unused) := access->call(source, Proxyref->Add, 1, sys->millisec() + 60000);
    completed <-= status;
}


# EN/RU: Own one independent variable actor through its terminal source clone cleanup.
# Владеть independent variable actor до terminal cleanup его source clone.
attach(route: Proxyroute, endpoint: ref Proxynet->Endpoint, image: Proxynet->Image,
       ready: chan of (int, ref Proxyref->Ref), done: chan of int)
{
    route->start(endpoint, image, 60000, ready);
    done <-= 1;
}


# EN/RU: Forward exactly eight borrowed requests; acknowledge only actual mailbox send.
# Передать ровно восемь borrowed requests; подтверждать лишь actual mailbox send.
forward(requests, mailbox: chan of ref Proxyref->Access, accepted: chan of int)
{
    for (index := 0; index < 8; index++) {
        request := <-requests;
        mailbox <-= request;
        accepted <-= 1;
    }
}


# EN: Establish a real connection, hold first completed effect, queue seven more actual requests,
# then require all eight callers to wake after disconnect. Omission cleanup drains only
# abandoned requests so the intentional semantic failure also joins every native child.
# RU: Создать actual connection, удержать first completed effect, поставить ещё семь actual requests,
# затем требовать wake всех восьми callers при disconnect. Omission cleanup снимает лишь
# abandoned requests, чтобы intentional semantic failure тоже завершил native children.
exercise(): int
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    net = load Proxynet Proxynet->PATH;
    net->init();
    ready := chan[1] of (int, ref Proxynet->Endpoint, Proxynet->Image, Proxynet->Image);
    spawn net->start(sys->fildes(0), sys->fildes(1), big 12345, big 67890, 60000, ready);
    (connected, endpoint, variable, function) := <-ready;
    if (connected != 0) {
        raise "disconnect waiters greeting";
    }
    access = load Proxyref Proxyref->PATH;
    access->init();
    route := load Proxyroute Proxyroute->PATH;
    route->init();
    attached := chan[1] of (int, ref Proxyref->Ref);
    done := chan[1] of int;
    spawn attach(route, endpoint, variable, attached, done);
    (status, source) := <-attached;
    if (status != 0) {
        raise "disconnect waiters attach";
    }
    mailbox := source.requests;
    source = ref Proxyref->Ref(chan[16] of ref Proxyref->Access, source.state, source.lease);
    accepted := chan[8] of int;
    spawn forward(source.requests, mailbox, accepted);
    started := chan[8] of int;
    completed := chan[8] of int;
    children := array[8] of int;
    spawn caller(source, started, completed);
    children[0] = <-started;
    deadline := sys->millisec() + 15000;
    gate := sys->open("disconnect-effect-held", Sys->OREAD);
    while (gate == nil && sys->millisec() < deadline) {
        sys->sleep(5);
        gate = sys->open("disconnect-effect-held", Sys->OREAD);
    }
    if (gate == nil) {
        raise "disconnect waiters first effect gate";
    }
    gate = nil;
    for (index := 1; index < 8; index++) {
        spawn caller(source, started, completed);
        children[index] = <-started;
    }
    for (index = 0; index < 8; index++) {
        if (<-accepted != 1) {
            raise "disconnect waiters actual offer acknowledgement";
        }
    }
    gate = sys->create("disconnect-queue-ready", Sys->OWRITE, 8r666);
    if (gate == nil) {
        raise "disconnect waiters queue publication";
    }
    gate = nil;
    deadline = sys->millisec() + 10000;
    while (!source.state.closed && sys->millisec() < deadline) {
        sys->sleep(5);
    }
    if (!source.state.closed) {
        raise "disconnect waiters original actor did not close";
    }
    deadline = sys->millisec() + 3000;
    count := 0;
    while (count < 8 && sys->millisec() < deadline) {
        alt {
            status := <-completed =>
                if (status != -3) {
                    raise "disconnect waiters offered outcome was not uncertain";
                }
                count++;
            * => sys->sleep(5);
        }
    }
    timely := count == 8;
    if (!timely) {
        for (;;) {
            request: ref Proxyref->Access;
            alt {
                request = <-mailbox => {}
                * => {}
            }
            if (request == nil) {
                break;
            }
            request.reply <-= (Proxyref->Unknown, 0);
        }
    }
    for (index = count; index < 8; index++) {
        if (<-completed != -3) {
            raise "disconnect waiters omission cleanup outcome";
        }
    }
    for (index = 0; index < 8; index++) {
        control->killwait(children[index]);
    }
    if (<-done != 1) {
        raise "disconnect waiters source actor cleanup";
    }
    if (timely) {
        sys->fprint(sys->fildes(2), "PASS disconnect waiters: eight offers woke before original sixty-second deadlines\n");
    }
    return timely;
}


# EN/RU: Release the exercise frame before reporting an intentional semantic omission.
# Освободить exercise frame до сообщения intentional semantic omission.
init()
{
    if (!exercise()) {
        raise "disconnect waiters did not observe terminal state";
    }
}
