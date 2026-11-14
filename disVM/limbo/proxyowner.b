# Copyright (c) 2026 bhgv
# EN: Execute local proxy operations at permanent owners with bounded deadlines.
# RU: Исполнять local proxy operations на постоянных owners с bounded deadlines.
implement Proxyowner;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
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


# EN: Own request/alarm to completion after public caller retirement. endpoint and
# request are borrowed immutable inputs; published/completed are buffered outputs.
# The original deadline and actual owner execution remain unchanged.
# RU: Владеть request/alarm до completion после retirement public caller. endpoint и
# request — borrowed immutable inputs, published/completed — buffered outputs.
# Original deadline и actual owner execution не меняются.
supervise(endpoint: ref Endpoint, request: ref Request, published: chan of int,
          completed: chan of (int, int, Handle))
{
    published <-= control->pid();
    absent := Handle(big 0, 0, big 0);
    deadline := request.deadline;
    signal := chan[1] of int;
    started := chan[1] of int;
    spawn alarm(signal, started, deadline);
    timer := <-started;
    delivered := 0;
    result := (NotSent, 0, absent);
    if (sys->millisec() < deadline) {
        alt {
            endpoint.requests <-= request => delivered = 1;
            <-signal => {}
        }
    }
    if (delivered) {
        alt {
            result = <-request.reply => {}
            <-signal => result = (Unknown, 0, absent);
        }
    }
    # EN/RU: One owned timer per request; completion never leaves a sleeping child.
    # Один owned timer на request; completion не оставляет спящего child.
    control->killwait(timer);
    completed <-= result;
}


# EN/RU: One absolute alarm spans offer and reply; accepted work is never replayed.
# Один alarm охватывает offer и reply; принятое work никогда не повторяется.
offer(endpoint: ref Endpoint, operation: int, target, left, right: Handle,
      refs, value, timeout, routed: int, first, second: ref Proxyref->Ref, publish: ref Proxyhost->Cell, deadline: int, output: ref RoutedResult): (int, int, Handle)
{
    absent := Handle(big 0, 0, big 0);
    if (endpoint == nil || timeout < 1 || timeout > 60000) {
        return (NotSent, 0, absent);
    }
    if (host->canwait() == 0) {
        return (OwnerError, 0, absent);
    }
    if (deadline == 0) {
        deadline = sys->millisec() + timeout;
    }
    response := chan[1] of (int, int, Handle);
    request := ref Request(operation, target, left, right, refs, value, routed, first, second, deadline, publish, output, response);
    published := chan[1] of int;
    completed := chan[1] of (int, int, Handle);
    spawn supervise(endpoint, request, published, completed);
    supervisor := <-published;
    result := <-completed;
    control->killwait(supervisor);
    return result;
}


# EN/RU: Ordinary scalar/lifecycle call; new export results require fetch instead.
# Обычный scalar/lifecycle call; new export results требуют fetch.
call(endpoint: ref Endpoint, operation: int, target, left, right: Handle,
     refs, value, timeout: int): (int, int)
{
    if ((operation < Read || operation > Retain) && operation != Add) {
        return (NotSent, 0);
    }
    (status, result, unused) := offer(endpoint, operation, target, left, right, refs, value, timeout, 0, nil, nil, nil, 0, nil);
    return (status, result);
}


# EN/RU: Typed result callback; only Variable/Function kinds authorize export.
# Typed result callback; лишь Variable/Function kinds разрешают export.
fetch(endpoint: ref Endpoint, target, left, right: Handle,
      refs, value, kind, timeout: int): (int, Handle)
{
    if (kind != Proxyhost->Variable && kind != Proxyhost->Function) {
        return (NotSent, Handle(big 0, 0, big 0));
    }
    (status, unused, returned) := offer(endpoint, ReturnVariable + kind,
        target, left, right, refs, value, timeout, 0, nil, nil, nil, 0, nil);
    return (status, returned);
}


# EN: Borrow endpoint and target/left/right handles plus refs/value until scalar
# completion. Preserve the absolute local deadline through request allocation;
# return status/value, refusing invalid/expired admission without native effects.
# RU: Borrow endpoint и target/left/right handles с refs/value до scalar completion.
# Сохранить absolute local deadline при request allocation; вернуть status/value,
# отказать invalid/expired admission без native effects.
calluntil(endpoint: ref Endpoint, operation: int, target, left, right: Handle,
          refs, value, deadline: int): (int, int)
{
    if ((operation < Read || operation > Retain) && operation != Add) {
        return (NotSent, 0);
    }
    (status, result, unused) := offer(endpoint, operation, target, left, right,
        refs, value, deadline - sys->millisec(), 0, nil, nil, nil, deadline, nil);
    return (status, result);
}


# EN: Borrow endpoint/target/left/right and refs/value under local absolute deadline.
# kind selects Variable/Function; success returns a new raw session-owned handle
# requiring explicit Drop, failure returns absent. Never renew callback admission.
# RU: Borrow endpoint/target/left/right и refs/value под local absolute deadline. kind
# выбирает Variable/Function; success возвращает new raw session-owned handle с
# explicit Drop, failure — absent. Не обновлять callback admission deadline.
fetchuntil(endpoint: ref Endpoint, target, left, right: Handle,
           refs, value, kind, deadline: int): (int, Handle)
{
    if (kind != Proxyhost->Variable && kind != Proxyhost->Function) {
        return (NotSent, Handle(big 0, 0, big 0));
    }
    (status, unused, returned) := offer(endpoint, ReturnVariable + kind,
        target, left, right, refs, value, deadline - sys->millisec(), 0, nil, nil, nil, deadline, nil);
    return (status, returned);
}


# EN/RU: Routed arguments stay managed and pinned by request; native pins capture only.
# Routed arguments остаются managed и удержаны request; native pins держат capture.
routed(endpoint: ref Endpoint, target: Handle, left, right: ref Proxyref->Ref,
       refs, value, timeout: int): (int, int)
{
    if (refs < 0 || refs > 2 || (refs > 0 && left == nil) || (refs > 1 && right == nil)) {
        return (NotSent, 0);
    }
    absent := Handle(big 0, 0, big 0);
    (status, result, unused) := offer(endpoint, Invoke, target, absent, absent,
        refs, value, timeout, 1, left, right, nil, 0, nil);
    return (status, result);
}


# EN/RU: Pin the original function while publishing a trusted local forwarding stub.
# Удерживать original function при публикации trusted local forwarding stub.
expose(endpoint: ref Endpoint, function: Handle, cell: ref Proxyhost->Cell, timeout: int): (int, Handle)
{
    absent := Handle(big 0, 0, big 0);
    (status, unused, returned) := offer(endpoint, Expose, function, absent, absent,
        0, 0, timeout, 0, nil, nil, cell, 0, nil);
    return (status, returned);
}


# EN/RU: Preserve the server's existing absolute deadline through routed argument allocation.
# Сохранить server absolute deadline при allocation routed arguments.
routeduntil(endpoint: ref Endpoint, target: Handle, left, right: ref Proxyref->Ref,
            refs, value, deadline: int): (int, int)
{
    if (refs < 0 || refs > 2 || (refs > 0 && left == nil) || (refs > 1 && right == nil)) {
        return (NotSent, 0);
    }
    absent := Handle(big 0, 0, big 0);
    (status, result, unused) := offer(endpoint, Invoke, target, absent, absent,
        refs, value, deadline - sys->millisec(), 1, left, right, nil, deadline, nil);
    return (status, result);
}


# EN/RU: Root a forwarding stub without renewing the original server deadline.
# Укоренить forwarding stub без обновления original server deadline.
exposeuntil(endpoint: ref Endpoint, function: Handle, cell: ref Proxyhost->Cell, deadline: int): (int, Handle)
{
    absent := Handle(big 0, 0, big 0);
    (status, unused, returned) := offer(endpoint, Expose, function, absent, absent,
        0, 0, deadline - sys->millisec(), 0, nil, nil, cell, deadline, nil);
    return (status, returned);
}


# EN: Execute result callback on original target owner with request-rooted refs.
# endpoint/target identify that owner; left/right and refs/value are borrowed inputs.
# Failure returns nil; successful nil callback values still require registry admission.
# The caller deadline covers allocation, queueing and completion.
# RU: Исполнить result callback на original target owner с request-rooted refs.
# endpoint/target задают owner; left/right и refs/value — borrowed inputs.
# При failure вернуть nil; successful nil callback values ещё проверяет registry.
# Caller deadline охватывает allocation, queueing и completion.
routedfetchuntil(endpoint: ref Endpoint, target: Handle, left, right: ref Proxyref->Ref,
                 refs, value, deadline: int): (int, ref Proxyref->Ref)
{
    if (refs < 0 || refs > 2 || (refs > 0 && left == nil) || (refs > 1 && right == nil)) {
        return (NotSent, nil);
    }
    output := ref RoutedResult(nil);
    absent := Handle(big 0, 0, big 0);
    (status, unused, ignored) := offer(endpoint, RoutedReturnVariable, target, absent, absent,
        refs, value, deadline - sys->millisec(), 1, left, right, nil, deadline, output);
    if (status != 0) {
        return (status, nil);
    }
    return (0, output.source);
}


# EN/RU: Registry retains an exact native result handle under its original deadline.
# Registry удерживает exact native result handle до original deadline.
retainuntil(endpoint: ref Endpoint, target: Handle, deadline: int): int
{
    absent := Handle(big 0, 0, big 0);
    (status, unused, ignored) := offer(endpoint, Retain, target, absent, absent,
        0, 0, deadline - sys->millisec(), 0, nil, nil, nil, deadline, nil);
    return status;
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
    absent := Handle(big 0, 0, big 0);
    for (;;) {
        request := <-endpoint.requests;
        if (request.deadline <= sys->millisec()) {
            request.reply <-= (NotSent, 0, absent);
            continue;
        }
        if (request.operation == Stop) {
            status = host->close();
            while (status == Proxyhost->Capacity) {
                sys->sleep(1);
                status = host->close();
            }
            request.reply <-= (status, 0, absent);
            return;
        }
        if (request.operation == Retain) {
            status = host->retain(request.target.session, request.target.slot, request.target.generation);
            request.reply <-= (status, 0, absent);
            continue;
        }
        if (request.operation == Drop) {
            status = host->drop(request.target.session, request.target.slot, request.target.generation);
            request.reply <-= (status, 0, absent);
            continue;
        }
        if (id == big 16r7fffffffffffffff) {
            raise "proxy request identity exhausted";
        }
        id++;
        nativeop := request.operation;
        if (nativeop == ReturnVariable || nativeop == ReturnFunction || nativeop == RoutedReturnVariable || nativeop == Expose) {
            nativeop = Invoke;
        }
        if (nativeop == Add) {
            nativeop = Proxyhost->Add;
        }
        native_refs := request.refs;
        if (request.routed) {
            native_refs = 0;
        }
        returned := absent;
        (admitted, target, left, right) := host->begin(id, nativeop,
            request.target.session, request.target.slot, request.target.generation,
            request.left.session, request.left.slot, request.left.generation,
            request.right.session, request.right.slot, request.right.generation, native_refs, request.value);
        value := 0;
        if (admitted == 0) {
            {
                case request.operation {
                    Read => value = target.value;
                    Write =>
                        target.value = request.value;
                        value = target.value;
                    Add =>
                        target.value += request.value;
                        value = target.value;
                    Invoke =>
                        if (request.routed) {
                            value = handler->routed(target, request.first, request.second,
                                request.refs, request.value, request.deadline);
                        } else {
                            value = handler->invoke(target, left, right, request.value);
                        }
                    RoutedReturnVariable =>
                        request.result.source = handler->routedresult(target, request.first, request.second,
                            request.refs, request.value, request.deadline);
                    Expose =>
                        (published, session, slot, generation) := host->publish(request.publish, Proxyhost->Variable);
                        admitted = published;
                        if (published == 0) {
                            returned = Handle(session, slot, generation);
                        }
                    ReturnVariable or ReturnFunction =>
                        kind := request.operation - ReturnVariable;
                        result := handler->result(target, left, right, request.value, kind);
                        (published, session, slot, generation) := host->publish(result, kind);
                        admitted = published;
                        if (published == 0) {
                            returned = Handle(session, slot, generation);
                        }
                        result = nil;
                }
            } exception {
                "*" => admitted = OwnerError;
            }
            if (host->finish(id) != 0) {
                raise "proxy owner finish";
            }
        }
        request.reply <-= (admitted, value, returned);
    }
}
