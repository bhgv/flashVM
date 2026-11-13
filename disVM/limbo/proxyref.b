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


# EN/RU: One finite alarm, publishing its PID and buffered timeout without waiting for receiver.
# Конечный alarm передаёт PID и buffered timeout без ожидания receiver.
alarm(started, signal: chan of int, deadline: int)
{
    started <-= control->pid();
    remaining := deadline - sys->millisec();
    if (remaining > 0) {
        sys->sleep(remaining);
    }
    signal <-= 1;
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
    started := chan[1] of int;
    signal := chan[1] of int;
    spawn alarm(started, signal, deadline);
    timer := <-started;
    offered := 0;
    result := (NotSent, 0);
    if (sys->millisec() < deadline) {
        alt {
            source.requests <-= access => offered = 1;
            <-signal => {}
        }
    }
    if (offered) {
        alt {
            result = <-reply => {}
            <-signal => result = (Unknown, 0);
        }
    }
    control->killwait(timer);
    return result;
}
