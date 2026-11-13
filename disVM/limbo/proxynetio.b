# Copyright (c) 2026 bhgv
# EN: Immutable network exchange and result validation, separate from the client actor.
# RU: Immutable network exchange и result validation отдельно от client actor.
implement Proxynetio;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxyframe.m";
include "proxystream.m";
include "proxynet.m";
include "proxynetio.m";
sys: Sys; # Immutable monotonic clock.
codec: Proxyframe; # Exact private record codec.
stream: Proxystream; # Bounded supervised native byte transfer.


# EN/RU: Initialize immutable dependencies before publishing the shared module.
# Инициализировать immutable dependencies до публикации shared module.
init()
{
    sys = load Sys Sys->PATH;
    codec = load Proxyframe Proxyframe->PATH;
    stream = load Proxystream Proxystream->PATH;
    if (sys == nil || codec == nil || stream == nil) {
        raise "proxy exchange modules unavailable";
    }
    stream->init();
}


# EN/RU: Encode exact scalar identities; no GC token or managed pointer enters wire.
# Кодировать точные scalar identities; GC token/managed pointer не попадают в wire.
requestbytes(request: ref Proxynet->Request, high, low, id: big): array of byte
{
    bytes := codec->make(Proxyframe->Call, high, low);
    codec->put(bytes, Proxyframe->Id, id);
    codec->put(bytes, Proxyframe->Operation, big (request.operation + 1));
    codec->put(bytes, Proxyframe->Refs, big request.refs);
    codec->put(bytes, Proxyframe->Value, big request.value);
    codec->sethandle(bytes, Proxyframe->Target, request.target.owner);
    codec->sethandle(bytes, Proxyframe->Left, request.left.owner);
    codec->sethandle(bytes, Proxyframe->Right, request.right.owner);
    return bytes;
}


# EN: Validate reply ownership extension independently of correlation. Malformed
# or unexpected handles are never adopted; failed/scalar replies require zero tail.
# RU: Проверить reply ownership extension отдельно от correlation. Malformed либо
# неожиданные handles не принимаются; failed/scalar replies требуют zero tail.
outcome(response: array of byte, request: ref Proxynet->Request): (int, Proxyowner->Handle)
{
    absent := Proxyowner->Handle(big 0, 0, big 0);
    kind := codec->get(response, Proxyframe->ResultKind);
    returned := absent;
    valid := 0;
    {
        returned = codec->handle(response, Proxyframe->Returned);
        resultcall := request.operation == Proxyowner->ReturnVariable || request.operation == Proxyowner->ReturnFunction || request.operation == Proxyowner->ImportRoute;
        if (resultcall && codec->get(response, Proxyframe->Status) == big 0) {
            expected := 1;
            if (request.operation == Proxyowner->ReturnFunction) {
                expected = 2;
            }
            valid = kind == big expected &&
                returned.session == request.target.owner.session && returned.session > big 0 &&
                returned.generation > big 0 && codec->get(response, Proxyframe->Result) == big 0;
        } else {
            valid = kind == big 0 && returned.session == big 0 && returned.slot == 0 && returned.generation == big 0;
        }
    } exception {
        "*" => valid = 0;
    }
    return (valid, returned);
}


# EN/RU: One absolute wire deadline; any incomplete/corrupt reply is terminal Unknown.
# Один absolute wire deadline; incomplete/corrupt reply означает terminal Unknown.
exchange(input, output: ref Sys->FD, request: ref Proxynet->Request, high, low, id: big, deadline: int): (int, int, int, Proxyowner->Handle)
{
    if (deadline <= sys->millisec()) {
        return (Proxyowner->NotSent, 0, 1, Proxyowner->Handle(big 0, 0, big 0));
    }
    bytes := requestbytes(request, high, low, id);
    (status, unused) := stream->transfer(output, bytes, 1, deadline);
    if (status == Proxystream->OK) {
        response := array[Proxyframe->Size] of byte;
        (status, unused) = stream->transfer(input, response, 0, deadline);
        if (status == Proxystream->OK && codec->valid(response, Proxyframe->Reply, high, low) &&
            codec->same(bytes, response, 1) && codec->get(response, Proxyframe->Status) >= big Proxyowner->Unknown &&
            codec->get(response, Proxyframe->Status) <= big 7) {
            (valid, handle) := outcome(response, request);
            if (valid) {
                returned := int codec->get(response, Proxyframe->Status);
                return (returned, int codec->get(response, Proxyframe->Result),
                    returned == Proxyowner->Unknown || returned == Proxyowner->NotSent, handle);
            }
        }
    }
    return (Proxyowner->Unknown, 0, 1, Proxyowner->Handle(big 0, 0, big 0));
}
