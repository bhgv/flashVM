# Copyright (c) 2026 bhgv
# EN: Independent variable-only access actors keep source images owned through in-flight work.
# RU: Independent variable-only access actors удерживают source images во время in-flight work.
implement Proxyroute;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyroute.m";
sys: Sys; # Immutable clock interface.
control: Processctl; # Immutable actor timer lifecycle interface.
net: Proxynet; # Independent remote requests run outside active function callbacks.
gc: Proxygc; # Native queue/token ownership; finalizers never perform IO.


# EN/RU: Initialize before any actors are spawned; dependencies remain immutable.
# Инициализировать до spawn actors; dependencies остаются immutable.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    net = load Proxynet Proxynet->PATH;
    gc = load Proxygc Proxygc->PATH;
    if (sys == nil || control == nil || net == nil || gc == nil) {
        raise "proxy route modules unavailable";
    }
    net->init();
}


# EN/RU: A finite timer publishes its PID and one buffered completion.
# Конечный timer публикует PID и один buffered completion.
alarm(started, signal: chan of int, deadline: int)
{
    started <-= control->pid();
    remaining := deadline - sys->millisec();
    if (remaining > 0) {
        sys->sleep(remaining);
    }
    signal <-= 1;
}


# EN: Serve only variable primitives. Requests hold source refs while queued or
# executing; GC owns a separate wrapper token, never rooted by this actor's loop.
# Return one if explicit Close already consumed the source image owner.
# RU: Обслуживать лишь variable primitives. Requests удерживают refs во время
# queue/execution; отдельный wrapper token не укоренён loop этого actor.
# Вернуть один, если explicit Close уже потребил source image owner.
run(source: ref Proxynet->Endpoint, image: Proxynet->Image, timeout: int,
    queue: ref Proxygc->Connection, requests: chan of ref Proxyref->Access, state: ref Proxyref->State): int
{
    idle := sys->millisec() + timeout;
    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    for (;;) {
        (session, unused, generation) := gc->next(queue);
        remaining := idle - sys->millisec();
        if (session != big 0 || remaining <= 0 || source.state.closed) {
            return 0;
        }
        if (remaining > 100) {
            remaining = 100;
        }
        started := chan[1] of int;
        signal := chan[1] of int;
        spawn alarm(started, signal, sys->millisec() + remaining);
        timer := <-started;
        access: ref Proxyref->Access;
        alt {
            access = <-requests => {}
            <-signal => {}
        }
        control->killwait(timer);
        if (access == nil) {
            continue;
        }
        status := Proxyref->Stale;
        value := 0;
        terminal := 0;
        closed := 0;
        remaining = access.deadline - sys->millisec();
        if (remaining > timeout) {
            remaining = timeout;
        }
        if (access.source != nil && gc->belongs(queue, access.source.lease) && gc->active(access.source.lease)) {
            if (remaining <= 0 || access.operation < Proxyref->Read || access.operation > Proxyref->Close) {
                status = Proxyref->NotSent;
            } else {
                operation := Proxyowner->Read;
                case access.operation {
                    Proxyref->Write => operation = Proxyowner->Write;
                    Proxyref->Add => operation = Proxyowner->Add;
                    Proxyref->Close => operation = Proxyowner->Drop;
                }
                if (access.operation == Proxyref->Close) {
                    closed = gc->consume(access.source.lease);
                }
                if (access.operation != Proxyref->Close || closed) {
                    (status, value) = net->call(source, operation, image, empty, empty, 0, access.value, remaining);
                    terminal = closed || status == Proxyref->Unknown || status == Proxyref->Disconnected;
                }
            }
        }
        if (terminal) {
            state.closed = 1;
            gc->close(queue);
        }
        access.reply <-= (status, value);
        access = nil;
        if (terminal) {
            return closed;
        }
        idle = sys->millisec() + timeout;
    }
}


# EN: Create a wrapper queue/token before Retain; drop all local greeting refs
# before waiting. Independent source image lives until actor cleanup, even when
# the attaching caller or a callback retires. No native pointer crosses a VM.
# RU: Создать wrapper queue/token до Retain; убрать local greeting refs до loop.
# Independent source image живёт до cleanup actor даже после attaching caller либо
# callback retirement. Native pointer не пересекает VM.
start(source: ref Proxynet->Endpoint, image: Proxynet->Image, timeout: int,
      ready: chan of (int, ref Proxyref->Ref))
{
    if (source == nil || timeout < 1 || timeout > 60000) {
        ready <-= (Proxyref->NotSent, nil);
        return;
    }
    queue := gc->open(1);
    if (queue == nil) {
        ready <-= (Proxyhost->Capacity, nil);
        return;
    }
    owner := image.owner;
    token := gc->make(queue, owner.session, owner.slot, owner.generation, 0);
    if (token == nil) {
        gc->close(queue);
        ready <-= (Proxyhost->Capacity, nil);
        return;
    }
    (status, copy) := net->clone(source, image, timeout);
    image = Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
    if (status != 0) {
        gc->close(queue);
        ready <-= (status, nil);
        return;
    }
    if (!gc->activate(token)) {
        raise "proxy route token activation refused";
    }
    requests := chan[16] of ref Proxyref->Access;
    state := ref Proxyref->State(0);
    wrapped := ref Proxyref->Ref(requests, state, token);
    ready <-= (0, wrapped);
    wrapped = nil;
    token = nil;
    released := 0;
    {
        released = run(source, copy, timeout, queue, requests, state);
    } exception {
        "*" => released = 0;
    }
    state.closed = 1;
    gc->close(queue);
    if (!released) {
        {
            net->call(source, Proxyowner->Drop, copy, image, image, 0, 0, timeout);
        } exception {
            "*" => {}
        }
    }
}
