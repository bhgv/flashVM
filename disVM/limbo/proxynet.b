# Copyright (c) 2026 bhgv
# EN: Client-side bounded single-flight wire broker with epoch/correlation fencing.
# RU: Клиентский bounded single-flight wire broker с epoch/correlation fencing.
implement Proxynet;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxyframe.m";
include "proxystream.m";
include "proxynet.m";
control: Processctl; # Immutable direct-child timer lifecycle.
sys: Sys; # Immutable system interface.
proxy: Proxyowner; # Immutable local offer/deadline interface.
codec: Proxyframe; # Immutable canonical codec.
stream: Proxystream; # Immutable bounded IO interface.


# EN/RU: Initialize module dependencies once before spawning a broker/client.
# Инициализировать зависимости один раз до spawn broker/client.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    proxy = load Proxyowner Proxyowner->PATH;
    codec = load Proxyframe Proxyframe->PATH;
    stream = load Proxystream Proxystream->PATH;
    if (control == nil || sys == nil || proxy == nil || codec == nil || stream == nil) {
        raise "proxy network modules unavailable";
    }
    proxy->init();
    stream->init();
}


# EN/RU: Check a caller-owned image against the endpoint's immutable incarnation.
# Проверить image caller относительно immutable incarnation endpoint.
current(endpoint: ref Endpoint, image: Image): int
{
    return image.high == endpoint.high && image.low == endpoint.low;
}


# EN/RU: Reject stale images, preserve alias handles, then offer to the local broker.
# Отказать stale images, сохранить alias handles и предложить call local broker.
call(endpoint: ref Endpoint, operation: int, target, left, right: Image,
     refs, value, timeout: int): (int, int)
{
    if (endpoint == nil || operation < Proxyowner->Read || operation > Proxyowner->Stop || refs < 0 || refs > 2) {
        return (Disconnected, 0);
    }
    if (endpoint.state.closed) {
        return (Disconnected, 0);
    }
    if ((operation != Proxyowner->Stop && !current(endpoint, target)) ||
        (refs > 0 && !current(endpoint, left)) || (refs > 1 && !current(endpoint, right))) {
        return (Stale, 0);
    }
    return proxy->call(endpoint.mailbox, operation, target.owner, left.owner, right.owner, refs, value, timeout);
}


# EN: Encode one immutable local request. The monotonically increasing id belongs
# only to this connection; response bytes must correlate before exposing status.
# RU: Кодировать immutable local request. Монотонный id принадлежит только этому
# соединению; response bytes проверяются до выдачи status.
requestbytes(request: ref Proxyowner->Request, high, low, id: big): array of byte
{
    bytes := codec->make(Proxyframe->Call, high, low);
    codec->put(bytes, Proxyframe->Id, id);
    codec->put(bytes, Proxyframe->Operation, big (request.operation + 1));
    codec->put(bytes, Proxyframe->Refs, big request.refs);
    codec->put(bytes, Proxyframe->Value, big request.value);
    codec->sethandle(bytes, Proxyframe->Target, request.target);
    codec->sethandle(bytes, Proxyframe->Left, request.left);
    codec->sethandle(bytes, Proxyframe->Right, request.right);
    return bytes;
}


# EN/RU: Finite local idle alarm; abandoned broker cannot retain endpoints forever.
# Конечный local idle alarm; abandoned broker не удерживает endpoints навсегда.
alarm(started, signal: chan of int, deadline: int)
{
    started <-= control->pid();
    remaining := deadline - sys->millisec();
    if (remaining > 0) {
        sys->sleep(remaining);
    }
    signal <-= 1;
}


# EN: Process sequential wire exchanges while callers run independently. Terminal
# failure permanently fences this broker; it never reconnects or retransmits.
# RU: Последовательно выполнять wire exchanges при независимых callers. Terminal
# failure навсегда закрывает broker; reconnect/retransmit не выполняются.
start(input, output: ref Sys->FD, high, low: big, timeout: int,
      ready: chan of (int, ref Endpoint, Image, Image))
{
    empty := Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0));
    if ((high == big 0 && low == big 0) || timeout < 1 || timeout > 60000) {
        ready <-= (Disconnected, nil, empty, empty);
        return;
    }
    hello := array[Proxyframe->Size] of byte;
    (status, unused) := stream->transfer(input, hello, 0, sys->millisec() + timeout);
    valid := 0;
    variable, function: Image;
    {
        if (status == Proxystream->OK && codec->valid(hello, Proxyframe->Hello, high, low) &&
            codec->get(hello, Proxyframe->Id) == big 0 &&
            codec->get(hello, Proxyframe->Operation) == big 0 &&
            codec->get(hello, Proxyframe->Refs) == big 0 &&
            codec->get(hello, Proxyframe->Value) == big 0 &&
            codec->get(hello, Proxyframe->Status) == big 0 &&
            codec->get(hello, Proxyframe->Result) == big 0) {
            variable = Image(high, low, codec->handle(hello, Proxyframe->Target));
            function = Image(high, low, codec->handle(hello, Proxyframe->Left));
            valid = variable.owner.session > big 0 && variable.owner.generation > big 0 &&
                function.owner.session == variable.owner.session && function.owner.generation > big 0;
        }
    } exception {
        "*" => valid = 0;
    }
    if (!valid) {
        ready <-= (Disconnected, nil, empty, empty);
        return;
    }
    endpoint := ref Endpoint(high, low, ref State(0), ref Proxyowner->Endpoint(chan[16] of ref Proxyowner->Request));
    ready <-= (0, endpoint, variable, function);
    id := big 0;
    closed := 0;
    for (;;) {
        started := chan[1] of int;
        signal := chan[1] of int;
        spawn alarm(started, signal, sys->millisec() + timeout);
        timer := <-started;
        request: ref Proxyowner->Request;
        alt {
            request = <-endpoint.mailbox.requests => {}
            <-signal => {}
        }
        control->killwait(timer);
        if (request == nil) {
            endpoint.state.closed = 1;
            return;
        }
        outcome := (Disconnected, 0);
        if (!closed && id < big 16r7fffffffffffffff) {
            id++;
            bytes := requestbytes(request, high, low, id);
            deadline := sys->millisec() + timeout;
            (status, unused) = stream->transfer(output, bytes, 1, deadline);
            if (status == Proxystream->OK) {
                response := array[Proxyframe->Size] of byte;
                (status, unused) = stream->transfer(input, response, 0, deadline);
                if (status == Proxystream->OK && codec->valid(response, Proxyframe->Reply, high, low) &&
                    codec->same(bytes, response, 1) &&
                    codec->get(response, Proxyframe->Status) >= big Proxyowner->Unknown &&
                    codec->get(response, Proxyframe->Status) <= big 7) {
                    returned := int codec->get(response, Proxyframe->Status);
                    outcome = (returned, int codec->get(response, Proxyframe->Result));
                    closed = returned == Proxyowner->Unknown || returned == Proxyowner->NotSent;
                } else {
                    closed = 1;
                    outcome = (Proxyowner->Unknown, 0);
                }
            } else {
                closed = 1;
                outcome = (Proxyowner->Unknown, 0);
            }
        } else {
            closed = 1;
        }
        if (closed || request.operation == Proxyowner->Stop) {
            endpoint.state.closed = 1;
        }
        request.reply <-= outcome;
        if (endpoint.state.closed) {
            return;
        }
    }
}
