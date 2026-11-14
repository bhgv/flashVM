# Copyright (c) 2026 bhgv
# EN: Finite variable-only offers; buffered completion owns in-flight proxy refs.
# RU: Конечные variable-only offers; buffered completion владеет in-flight proxy refs.
implement Proxyref;
include "sys.m";
include "processctl.m";
include "proxyref.m";
sys: Sys; # Immutable local monotonic clock interface.
control: Processctl; # Immutable child retirement interface.
gc: Proxygc; # Exact native token activation query.


# EN/RU: Initialize before sharing this module; no per-call global mutation.
# Инициализировать до sharing модуля; без per-call global mutation.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    gc = load Proxygc Proxygc->PATH;
    if (sys == nil || control == nil || gc == nil) {
        raise "proxy ref modules unavailable";
    }
}


# EN: One watchdog borrows state/deadline; observe closure within one polling slice
# without renewing the original deadline. Publish PID and buffered wakeup.
# RU: Один watchdog заимствует state/deadline; увидеть closure за один polling slice
# без обновления original deadline. Передать PID и buffered wakeup.
watch(state: ref State, started, signal: chan of int, deadline: int)
{
    started <-= control->pid();
    while (!state.closed) {
        remaining := deadline - sys->millisec();
        if (remaining <= 0) {
            break;
        }
        if (remaining > 100) {
            remaining = 100;
        }
        sys->sleep(remaining);
    }
    signal <-= 1;
}


# EN: Keep access/source and original deadline until reply, closure or timeout even
# after caller retirement. Borrow access, publish PID/result to buffered outputs;
# always join the owned watchdog before completion, preferring an available reply.
# RU: Удержать access/source и original deadline до reply, closure либо timeout даже
# после retirement caller. Borrow access, передать PID/result в buffered outputs;
# всегда join owned watchdog до completion, с приоритетом доступного reply.
supervise(access: ref Access, published: chan of int, completed: chan of (int, int))
{
    published <-= control->pid();
    source := access.source;
    deadline := access.deadline;
    started := chan[1] of int;
    signal := chan[1] of int;
    spawn watch(source.state, started, signal, deadline);
    watchdog := <-started;
    offered := 0;
    result := (NotSent, 0);
    if (!source.state.closed && sys->millisec() < deadline) {
        alt {
            source.requests <-= access => offered = 1;
            <-signal => {}
        }
    }
    if (!offered && source.state.closed) {
        result = (Disconnected, 0);
    }
    if (offered) {
        alt {
            result = <-access.reply => {}
            <-signal =>
                alt {
                    result = <-access.reply => {}
                    * => result = (Unknown, 0);
                }
        }
    }
    control->killwait(watchdog);
    completed <-= result;
}


# EN/RU: Keep source rooted in queued Access; never turn an offered Unknown into retry.
# Удерживать source через queued Access; offered Unknown никогда не становится retry.
call(source: ref Ref, operation, value, deadline: int): (int, int)
{
    remaining := deadline - sys->millisec();
    if (source == nil || operation < Read || operation > Close || remaining < 1 || remaining > 60000) {
        return (NotSent, 0);
    }
    if (source.state.closed) {
        return (Disconnected, 0);
    }
    if (!gc->active(source.lease)) {
        return (Stale, 0);
    }
    reply := chan[1] of (int, int);
    access := ref Access(source, operation, value, deadline, reply);
    published := chan[1] of int;
    completed := chan[1] of (int, int);
    spawn supervise(access, published, completed);
    supervisor := <-published;
    result := <-completed;
    control->killwait(supervisor);
    return result;
}
