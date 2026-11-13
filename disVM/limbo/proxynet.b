# Copyright (c) 2026 bhgv
# EN: Single-flight client with counted GC image owners, epoch fencing and finite IO.
# RU: Single-flight client с counted GC image owners, epoch fencing и конечным IO.
implement Proxynet;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxyframe.m";
include "proxystream.m";
include "proxynet.m";
control: Processctl; # Immutable child lifecycle interface.
sys: Sys; # Immutable clock interface.
host: Proxyhost; # Exact native callback wait policy.
gc: Proxygc; # Native metadata/token ownership, no network work in finalizers.
codec: Proxyframe; # Canonical private wire codec.
stream: Proxystream; # Finite supervised byte IO.


# EN/RU: Initialize all immutable dependencies before sharing module with Progs.
# Инициализировать immutable dependencies до sharing module с Progs.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    host = load Proxyhost Proxyhost->PATH;
    gc = load Proxygc Proxygc->PATH;
    codec = load Proxyframe Proxyframe->PATH;
    stream = load Proxystream Proxystream->PATH;
    if (sys == nil || control == nil || host == nil || gc == nil || codec == nil || stream == nil) {
        raise "proxy network modules unavailable";
    }
    stream->init();
}


# EN/RU: Return a scalar-only absent image with no managed lease owner.
# Вернуть отсутствующий image без managed lease owner.
empty(): Image
{
    return Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);
}


# EN/RU: Check current epoch and active logical lease, including after queued Release.
# Проверить current epoch и active logical lease, включая queued Release.
current(endpoint: ref Endpoint, image: Image): int
{
    return image.high == endpoint.high && image.low == endpoint.low && gc->matches(image.lease, image.owner.session, image.owner.slot, image.owner.generation);
}


# EN/RU: Finite alarm owns its buffered signal; abandoned caller cannot strand sender.
# Конечный alarm владеет buffered signal; abandoned caller не блокирует sender.
alarm(started, signal: chan of int, deadline: int)
{
    started <-= control->pid();
    remaining := deadline - sys->millisec();
    if (remaining > 0) {
        sys->sleep(remaining);
    }
    signal <-= 1;
}


# EN: Offer immutable request with one overall deadline. Request/reply keep every
# lease rooted; an abandoned successful clone is finalized from its orphan reply.
# RU: Предложить immutable request с общим deadline. Request/reply держат leases;
# abandoned successful clone финализируется из orphan reply.
offer(endpoint: ref Endpoint, operation: int, target, left, right: Image,
      refs, value, timeout: int): Reply
{
    absent := empty();
    if (endpoint == nil || timeout < 1 || timeout > 60000 || refs < 0 || refs > 2) {
        return Reply(Proxyowner->NotSent, 0, absent);
    }
    if (!host->canwait()) {
        return Reply(Proxyowner->OwnerError, 0, absent);
    }
    if (endpoint.state.closed) {
        return Reply(Disconnected, 0, absent);
    }
    if ((operation != Proxyowner->Stop && operation != GCProgress && !current(endpoint, target)) ||
        (refs > 0 && !current(endpoint, left)) || (refs > 1 && !current(endpoint, right))) {
        return Reply(Stale, 0, absent);
    }
    response := chan[1] of Reply;
    request := ref Request(operation, target, left, right, refs, value, response);
    started := chan[1] of int;
    signal := chan[1] of int;
    deadline := sys->millisec() + timeout;
    spawn alarm(started, signal, deadline);
    timer := <-started;
    delivered := 0;
    result := Reply(Proxyowner->NotSent, 0, absent);
    if (sys->millisec() < deadline) {
        alt {
            endpoint.requests <-= request => delivered = 1;
            <-signal => {}
        }
    }
    if (delivered) {
        alt {
            result = <-response => {}
            <-signal => result = Reply(Proxyowner->Unknown, 0, absent);
        }
    }
    control->killwait(timer);
    return result;
}


# EN/RU: Run ordinary scalar operation; explicit Drop consumes this logical image once.
# Исполнить scalar operation; explicit Drop потребляет logical image один раз.
call(endpoint: ref Endpoint, operation: int, target, left, right: Image,
     refs, value, timeout: int): (int, int)
{
    if (operation < Proxyowner->Read || operation > Proxyowner->Stop) {
        return (Disconnected, 0);
    }
    result := offer(endpoint, operation, target, left, right, refs, value, timeout);
    return (result.status, result.value);
}


# EN/RU: Request a distinct counted/GC-finalized remote owner without changing source lease.
# Запросить отдельный counted/GC-finalized remote owner без изменения source lease.
clone(endpoint: ref Endpoint, source: Image, timeout: int): (int, Image)
{
    absent := empty();
    result := offer(endpoint, Proxyowner->Retain, source, absent, absent, 0, 0, timeout);
    return (result.status, result.image);
}


# EN/RU: Read broker's cumulative acknowledged GC Release count; no forced collection.
# Читать cumulative acknowledged GC Release count broker; без forced collection.
collected(endpoint: ref Endpoint, timeout: int): (int, int)
{
    absent := empty();
    result := offer(endpoint, GCProgress, absent, absent, absent, 0, 0, timeout);
    return (result.status, result.value);
}


# EN/RU: Encode exact scalar identities; no GC token or managed pointer enters wire.
# Кодировать точные scalar identities; GC token/managed pointer не попадают в wire.
requestbytes(request: ref Request, high, low, id: big): array of byte
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


# EN/RU: One absolute wire deadline; any incomplete/corrupt reply is terminal Unknown.
# Один absolute wire deadline; incomplete/corrupt reply означает terminal Unknown.
exchange(input, output: ref Sys->FD, request: ref Request, high, low, id: big, timeout: int): (int, int, int)
{
    bytes := requestbytes(request, high, low, id);
    deadline := sys->millisec() + timeout;
    (status, unused) := stream->transfer(output, bytes, 1, deadline);
    if (status == Proxystream->OK) {
        response := array[Proxyframe->Size] of byte;
        (status, unused) = stream->transfer(input, response, 0, deadline);
        if (status == Proxystream->OK && codec->valid(response, Proxyframe->Reply, high, low) &&
            codec->same(bytes, response, 1) && codec->get(response, Proxyframe->Status) >= big Proxyowner->Unknown &&
            codec->get(response, Proxyframe->Status) <= big 7) {
            returned := int codec->get(response, Proxyframe->Status);
            return (returned, int codec->get(response, Proxyframe->Result),
                returned == Proxyowner->Unknown || returned == Proxyowner->NotSent);
        }
    }
    return (Proxyowner->Unknown, 0, 1);
}


# EN: Drain bounded collector metadata between calls. Finalizers never perform IO.
# Pending records contain no managed tokens; popped Release is sent once only.
# RU: Снимать bounded collector metadata между calls. Finalizers не выполняют IO.
# Pending records не содержат managed tokens; popped Release отправляется один раз.
run(input, output: ref Sys->FD, endpoint: ref Endpoint, connection: ref Proxygc->Connection, timeout: int)
{
    absent := empty();
    id := big 0;
    idle := sys->millisec() + timeout;
    closed := 0;
    released := 0;
    for (;;) {
        remaining := idle - sys->millisec();
        if (remaining <= 0 || id == big 16r7fffffffffffffff) {
            break;
        }
        (session, slot, generation) := gc->next(connection);
        if (session != big 0) {
            target := Image(endpoint.high, endpoint.low, Proxyowner->Handle(session, slot, generation), nil);
            release := ref Request(Proxyowner->Drop, target, absent, absent, 0, 0, nil);
            id++;
            (status, unused, terminal) := exchange(input, output, release, endpoint.high, endpoint.low, id, remaining);
            release = nil;
            if (terminal || (status != 0 && status != Stale) || id == big 16r7fffffffffffffff) {
                break;
            }
            released++;
        }
        started := chan[1] of int;
        signal := chan[1] of int;
        delay := idle - sys->millisec();
        if (delay > 100) {
            delay = 100;
        }
        spawn alarm(started, signal, sys->millisec() + delay);
        timer := <-started;
        request: ref Request;
        alt {
            request = <-endpoint.requests => {}
            <-signal => {}
        }
        control->killwait(timer);
        if (request == nil) {
            continue;
        }
        result := Reply(Stale, 0, absent);
        staged: ref Proxygc->Lease;
        admitted := (request.operation == Proxyowner->Stop || request.operation == GCProgress ||
            current(endpoint, request.target)) &&
            (request.refs < 1 || current(endpoint, request.left)) &&
            (request.refs < 2 || current(endpoint, request.right));
        if (admitted && request.operation == Proxyowner->Retain) {
            owner := request.target.owner;
            staged = gc->make(connection, owner.session, owner.slot, owner.generation, 0);
            if (staged == nil) {
                admitted = 0;
                result.status = Proxyhost->Capacity;
            }
        }
        if (admitted && request.operation == Proxyowner->Drop) {
            admitted = gc->consume(request.target.lease);
        }
        if (admitted && request.operation == GCProgress) {
            result = Reply(0, released, absent);
        } else if (admitted) {
            id++;
            (result.status, result.value, closed) = exchange(input, output, request,
                endpoint.high, endpoint.low, id, timeout);
            if (!closed && result.status == 0 && request.operation == Proxyowner->Retain) {
                if (!gc->activate(staged)) {
                    raise "proxy staged lease activation refused";
                }
                result.image = request.target;
                result.image.lease = staged;
            }
        }
        if (closed || request.operation == Proxyowner->Stop) {
            endpoint.state.closed = 1;
            gc->close(connection);
        }
        request.reply <-= result;
        request = nil;
        staged = nil;
        result = Reply(0, 0, absent);
        if (endpoint.state.closed) {
            return;
        }
        idle = sys->millisec() + timeout;
    }
    endpoint.state.closed = 1;
    gc->close(connection);
}


# EN: Validate greeting before allocating active image tokens; private queue
# closes on every terminal path. Drop local greeting copies before broker waits.
# RU: Проверить greeting до active image tokens; private queue закрывается на
# каждом terminal path. Снять local greeting copies до ожидания broker.
start(input, output: ref Sys->FD, high, low: big, timeout: int,
      ready: chan of (int, ref Endpoint, Image, Image))
{
    absent := empty();
    if ((high == big 0 && low == big 0) || timeout < 1 || timeout > 60000) {
        ready <-= (Disconnected, nil, absent, absent);
        return;
    }
    hello := array[Proxyframe->Size] of byte;
    (status, unused) := stream->transfer(input, hello, 0, sys->millisec() + timeout);
    valid := 0;
    variable, function: Image;
    {
        if (status == Proxystream->OK && codec->valid(hello, Proxyframe->Hello, high, low) &&
            codec->get(hello, Proxyframe->Id) == big 0 && codec->get(hello, Proxyframe->Operation) == big 0 &&
            codec->get(hello, Proxyframe->Refs) == big 0 && codec->get(hello, Proxyframe->Value) == big 0 &&
            codec->get(hello, Proxyframe->Status) == big 0 && codec->get(hello, Proxyframe->Result) == big 0) {
            variable = Image(high, low, codec->handle(hello, Proxyframe->Target), nil);
            function = Image(high, low, codec->handle(hello, Proxyframe->Left), nil);
            valid = variable.owner.session > big 0 && variable.owner.generation > big 0 &&
                function.owner.session == variable.owner.session && function.owner.generation > big 0;
        }
    } exception {
        "*" => valid = 0;
    }
    if (!valid) {
        ready <-= (Disconnected, nil, absent, absent);
        return;
    }
    connection := gc->open(256);
    if (connection == nil) {
        ready <-= (Proxyhost->Capacity, nil, absent, absent);
        return;
    }
    variable.lease = gc->make(connection, variable.owner.session, variable.owner.slot, variable.owner.generation, 1);
    function.lease = gc->make(connection, function.owner.session, function.owner.slot, function.owner.generation, 1);
    if (variable.lease == nil || function.lease == nil) {
        gc->close(connection);
        ready <-= (Proxyhost->Capacity, nil, absent, absent);
        return;
    }
    endpoint := ref Endpoint(high, low, ref State(0), chan[16] of ref Request);
    ready <-= (0, endpoint, variable, function);
    variable = absent;
    function = absent;
    {
        run(input, output, endpoint, connection, timeout);
    } exception {
        "*" =>
            endpoint.state.closed = 1;
            gc->close(connection);
            raise;
    }
}
