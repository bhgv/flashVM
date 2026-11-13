# Copyright (c) 2026 bhgv
# EN: Execute local proxy operations at permanent owners with bounded deadlines.
# RU: Исполнять local proxy operations на постоянных owners с bounded deadlines.
implement Proxyowner;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyhandler.m";
include "proxyowner.m";

sys: Sys; # Immutable system interface after init.
host: Proxyhost; # Immutable owner-bound builtin interface after init.
control: Processctl; # Immutable direct-child lifecycle interface.


# EN/RU: Initialize builtin references once before exposing the shared library.
# Инициализировать builtin references один раз до передачи общей библиотеки.
init()
{
    sys = load Sys Sys->PATH;
    host = load Proxyhost Proxyhost->PATH;
    control = load Processctl Processctl->PATH;
    if (sys == nil || host == nil || control == nil) {
        raise "proxy owner builtins unavailable";
    }
}


# EN: Publish a timeout to a one-slot channel, then retire. There is no waiting
# receiver requirement; a timed-out/cancelled caller cannot leave this sender stuck.
# RU: Передать timeout в one-slot channel и завершиться. Получатель не обязателен;
# timeout/cancellation caller не оставляет sender заблокированным.
alarm(signal, started: chan of int, deadline: int)
{
    started <-= control->pid();
    remaining := deadline - sys->millisec();
    if (remaining > 0) {
        sys->sleep(remaining);
    }
    signal <-= 1;
}


# EN/RU: One absolute alarm spans offer and reply; accepted work is never replayed.
# Один alarm охватывает offer и reply; принятое work никогда не повторяется.
call(endpoint: ref Endpoint, operation: int, target, left, right: Handle,
     refs, value, timeout: int): (int, int)
{
    if (endpoint == nil || timeout < 1 || timeout > 60000) {
        return (NotSent, 0);
    }
    if (host->canwait() == 0) {
        return (OwnerError, 0);
    }
    signal := chan[1] of int;
    response := chan[1] of (int, int);
    started := chan[1] of int;
    request := ref Request(operation, target, left, right, refs, value, response);
    deadline := sys->millisec() + timeout;
    spawn alarm(signal, started, deadline);
    timer := <-started;
    delivered := 0;
    result := (NotSent, 0);
    if (sys->millisec() < deadline) {
        alt {
            endpoint.requests <-= request => delivered = 1;
            <-signal => {}
        }
    }
    if (delivered) {
        alt {
            result = <-response => {}
            <-signal => result = (Unknown, 0);
        }
    }
    # EN/RU: One timer per active caller; fast calls never leave sleeping children.
    # Один timer на active caller; быстрые calls не оставляют спящих детей.
    control->killwait(timer);
    return result;
}


# EN: Keep actual roots/callback execution on this Prog. Native session IDs count
# processed local offers, not a network dedup protocol. Catch callback exceptions,
# finish pins before replying, and never replay a failed/unknown effect.
# RU: Хранить actual roots/исполнение на этом Prog. Native IDs считают обработанные
# local offers, не являются network dedup. Перехватывать callback errors, завершать
# pins до reply и не повторять failed/unknown effects.
serve(handler: Proxyhandler, cell, capture: ref Proxyhost->Cell,
      ready: chan of (ref Endpoint, Handle, Handle))
{
    if (host->open(16, 64) != 0) {
        raise "proxy owner session open";
    }
    handler->start(capture);
    (status, session, slot, generation) := host->publish(cell, Proxyhost->Variable);
    if (status != 0) {
        raise "proxy variable publish";
    }
    variable := Handle(session, slot, generation);
    (status, session, slot, generation) = host->publish(capture, Proxyhost->Function);
    if (status != 0) {
        raise "proxy function publish";
    }
    function := Handle(session, slot, generation);
    endpoint := ref Endpoint(chan[16] of ref Request);
    ready <-= (endpoint, variable, function);
    id := big 0;
    for (;;) {
        request := <-endpoint.requests;
        if (request.operation == Stop) {
            status = host->close();
            while (status == Proxyhost->Capacity) {
                sys->sleep(1);
                status = host->close();
            }
            request.reply <-= (status, 0);
            return;
        }
        if (request.operation == Drop) {
            status = host->drop(request.target.session, request.target.slot, request.target.generation);
            request.reply <-= (status, 0);
            continue;
        }
        if (id == big 16r7fffffffffffffff) {
            raise "proxy request identity exhausted";
        }
        id++;
        (admitted, target, left, right) := host->begin(id, request.operation,
            request.target.session, request.target.slot, request.target.generation,
            request.left.session, request.left.slot, request.left.generation,
            request.right.session, request.right.slot, request.right.generation, request.refs, request.value);
        value := 0;
        if (admitted == 0) {
            {
                case request.operation {
                    Read => value = target.value;
                    Write =>
                        target.value = request.value;
                        value = target.value;
                    Invoke => value = handler->invoke(target, left, right, request.value);
                }
            } exception {
                "*" => admitted = OwnerError;
            }
            if (host->finish(id) != 0) {
                raise "proxy owner finish";
            }
        }
        request.reply <-= (admitted, value);
    }
}
