# Copyright (c) 2026 bhgv
# EN: Single-connection typed proxy bridge with bounded exact response replay and owner cleanup.
# RU: Typed proxy bridge одного соединения с bounded exact response replay и owner cleanup.
implement Proxyserver;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxyframe.m";
include "proxystream.m";
include "proxynet.m";
include "proxyregistry.m";
include "proxyserver.m";
sys: Sys; # Immutable system interface.
control: Processctl; # Immutable child lifecycle interface.
proxy: Proxyowner; # Immutable managed owner adapter.
codec: Proxyframe; # Immutable private wire codec.
registry: Proxyregistry; # Connection-scoped forwarding authority.
stream: Proxystream; # Immutable bounded IO adapter.


# EN/RU: Initialize before sharing module with connection coordinators.
# Инициализировать до sharing module с connection coordinators.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    proxy = load Proxyowner Proxyowner->PATH;
    codec = load Proxyframe Proxyframe->PATH;
    stream = load Proxystream Proxystream->PATH;
    registry = load Proxyregistry Proxyregistry->PATH;
    if (sys == nil || control == nil || proxy == nil || codec == nil || stream == nil || registry == nil) {
        raise "proxy server modules unavailable";
    }
    proxy->init();
    stream->init();
    registry->init();
}


# EN/RU: Publish this child's PID before entering the actual original owner loop.
# Опубликовать PID этого child до входа в настоящий original owner loop.
own(handler: Proxyhandler, cell, capture: ref Proxyhost->Cell, started: chan of int,
    ready: chan of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle))
{
    started <-= control->pid();
    proxy->serve(handler, cell, capture, ready);
}


# EN/RU: Reject noncanonical call metadata before allocating managed call arguments.
# Отказать неканоническим call metadata до создания managed call arguments.
canonical(bytes: array of byte): int
{
    operation := codec->get(bytes, Proxyframe->Operation);
    refs := codec->get(bytes, Proxyframe->Refs);
    return codec->get(bytes, Proxyframe->Id) > big 0 && operation >= big 1 && operation <= big 12 &&
        refs >= big 0 && refs <= big 2 && codec->get(bytes, Proxyframe->Status) == big 0 &&
        codec->get(bytes, Proxyframe->Result) == big 0 &&
        codec->get(bytes, Proxyframe->ResultKind) == big 0 &&
        codec->get(bytes, Proxyframe->Returned) == big 0 &&
        codec->get(bytes, Proxyframe->Returned + 8) == big 0 &&
        codec->get(bytes, Proxyframe->Returned + 16) == big 0;
}


# EN: Serve fixed requests sequentially; only next id authorizes a new owner
# effect. Last exact request returns cached result, changed reuse conflicts,
# older IDs are stale. A cache overwrite never permits an older effect again.
# RU: Обслуживать fixed requests последовательно; лишь следующий id разрешает
# новый effect. Точный last request получает cached result, changed reuse —
# conflict, старые IDs — stale. Смена cache не разрешает старый effect вновь.
run(input, output: ref Sys->FD, high, low: big, timeout: int,
    endpoint: ref Proxyowner->Endpoint, variable, function: Proxyowner->Handle, context: ref Proxyregistry->Context): int
{
    hello := codec->make(Proxyframe->Hello, high, low);
    codec->sethandle(hello, Proxyframe->Target, variable);
    codec->sethandle(hello, Proxyframe->Left, function);
    (io, unused) := stream->transfer(output, hello, 1, sys->millisec() + timeout);
    if (io != Proxystream->OK) {
        return io;
    }
    last := big 0;
    previous, cached: array of byte;
    for (;;) {
        bytes := array[Proxyframe->Size] of byte;
        deadline := sys->millisec() + timeout;
        (io, unused) = stream->transfer(input, bytes, 0, deadline);
        if (io != Proxystream->OK) {
            return io;
        }
        if (!codec->valid(bytes, Proxyframe->Call, high, low) || !canonical(bytes)) {
            return Proxystream->Failure;
        }
        id := codec->get(bytes, Proxyframe->Id);
        status := 5;
        value := 0;
        response: array of byte;
        stop := 0;
        if (id == last) {
            if (codec->same(bytes, previous, 0)) {
                response = cached;
            } else {
                response = codec->answer(bytes, 6, 0);
            }
        } else if (last < big 16r7fffffffffffffff && id == last + big 1) {
            operation := int codec->get(bytes, Proxyframe->Operation) - 1;
            remaining := deadline - sys->millisec();
            if (remaining < 1) {
                return Proxystream->Timeout;
            }
            returned := Proxyowner->Handle(big 0, 0, big 0);
            resultkind := 0;
            handled: int;
            (handled, status, value, returned, resultkind) = registry->dispatch(context, operation,
                codec->handle(bytes, Proxyframe->Target), codec->handle(bytes, Proxyframe->Left),
                codec->handle(bytes, Proxyframe->Right), int codec->get(bytes, Proxyframe->Refs),
                int codec->get(bytes, Proxyframe->Value), deadline);
            remaining = deadline - sys->millisec();
            if (!handled && remaining < 1) {
                return Proxystream->Timeout;
            }
            if (!handled && (operation == Proxyowner->ReturnVariable || operation == Proxyowner->ReturnFunction)) {
                kind := operation - Proxyowner->ReturnVariable;
                (status, returned) = proxy->fetchuntil(endpoint,
                    codec->handle(bytes, Proxyframe->Target), codec->handle(bytes, Proxyframe->Left),
                    codec->handle(bytes, Proxyframe->Right), int codec->get(bytes, Proxyframe->Refs),
                    int codec->get(bytes, Proxyframe->Value), kind, deadline);
                if (status == 0) {
                    resultkind = kind + 1;
                }
            } else if (!handled) {
                (status, value) = proxy->calluntil(endpoint, operation,
                    codec->handle(bytes, Proxyframe->Target), codec->handle(bytes, Proxyframe->Left),
                    codec->handle(bytes, Proxyframe->Right), int codec->get(bytes, Proxyframe->Refs),
                    int codec->get(bytes, Proxyframe->Value), deadline);
            }
            response = codec->answer(bytes, status, value);
            codec->put(response, Proxyframe->ResultKind, big resultkind);
            codec->sethandle(response, Proxyframe->Returned, returned);
            previous = bytes;
            cached = response;
            last = id;
            stop = operation == Proxyowner->Stop || status == Proxyowner->Unknown || status == Proxyowner->NotSent;
        } else {
            response = codec->answer(bytes, status, value);
        }
        (io, unused) = stream->transfer(output, response, 1, deadline);
        if (io != Proxystream->OK || stop) {
            return io;
        }
    }
}


# EN: A private session owns execution until EOF/deadline/Stop even if its public
# caller retires. Join the original rooted owner on every terminal path.
# RU: Private session владеет execution до EOF/deadline/Stop даже при retirement
# public caller. На каждом terminal path join исходного rooted owner.
session(input, output: ref Sys->FD, high, low: big, timeout: int,
        handler: Proxyhandler, cell, capture: ref Proxyhost->Cell,
        sources: array of ref Proxyregistry->Source, published: chan of int, completed: chan of int)
{
    published <-= control->pid();
    started := chan[1] of int;
    ready := chan[1] of (ref Proxyowner->Endpoint, Proxyowner->Handle, Proxyowner->Handle);
    spawn own(handler, cell, capture, started, ready);
    owner := <-started;
    (endpoint, variable, function) := <-ready;
    context := registry->create(sources, endpoint, timeout);
    if (sources != nil && context == nil) {
        control->killwait(owner);
        completed <-= Proxystream->Failure;
        return;
    }
    result := Proxystream->Failure;
    {
        result = run(input, output, high, low, timeout, endpoint, variable, function, context);
    } exception {
        "*" => result = Proxystream->Failure;
    }
    control->killwait(owner);
    registry->close(context);
    completed <-= result;
}


# EN/RU: Wait for independently supervised connection; borrow FD/roots until completion.
# Ждать независимо supervised connection; заимствовать FD/roots до completion.
registered(input, output: ref Sys->FD, high, low: big, timeout: int,
           handler: Proxyhandler, cell, capture: ref Proxyhost->Cell,
           sources: array of ref Proxyregistry->Source): int
{
    if ((high == big 0 && low == big 0) || timeout < 1 || timeout > 60000 || handler == nil) {
        return Proxystream->Failure;
    }
    published := chan[1] of int;
    completed := chan[1] of int;
    spawn session(input, output, high, low, timeout, handler, cell, capture, sources, published, completed);
    supervisor := <-published;
    result := <-completed;
    control->killwait(supervisor);
    return result;
}


# EN/RU: Ordinary server has no forwarding authority; preserve its existing API.
# Ordinary server не имеет forwarding authority; сохранить существующий API.
serve(input, output: ref Sys->FD, high, low: big, timeout: int,
      handler: Proxyhandler, cell, capture: ref Proxyhost->Cell): int
{
    return registered(input, output, high, low, timeout, handler, cell, capture, nil);
}
