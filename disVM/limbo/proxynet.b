# Copyright (c) 2026 bhgv
# EN: Single-flight client with counted GC image owners, epoch fencing and finite IO.
# RU: Single-flight client с counted GC image owners, epoch fencing и конечным IO.
implement Proxynet;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxyframe.m";
include "proxystream.m";
include "proxynet.m";
include "proxynetio.m";
control: Processctl; # Immutable child lifecycle interface.
sys: Sys; # Immutable clock interface.
host: Proxyhost; # Exact native callback wait policy.
gc: Proxygc; # Native metadata/token ownership, no network work in finalizers.
codec: Proxyframe; # Canonical private wire codec.
io: Proxynetio; # Immutable wire exchange implementation.
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
    io = load Proxynetio Proxynetio->PATH;
    if (sys == nil || control == nil || host == nil || gc == nil || codec == nil || stream == nil || io == nil) {
        raise "proxy network modules unavailable";
    }
    stream->init();
    io->init();
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


# EN: One watchdog borrows endpoint/deadline and signals at terminal state or the
# original deadline. Polling never renews the budget and never closes/replays IO.
# RU: Один watchdog заимствует endpoint/deadline и сообщает terminal state либо
# original deadline. Polling не обновляет budget и не закрывает/повторяет IO.
watch(endpoint: ref Endpoint, started, signal: chan of int, deadline: int)
{
    started <-= control->pid();
    while (!endpoint.state.closed) {
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


# EN: Own watchdog/completion through request completion or observed closure even
# after caller retirement. Borrow endpoint/request/deadline; the broker owns mutable
# terminal state. published/completed are buffered outputs. Never renew/replay work.
# RU: Владеть watchdog/completion до request completion либо observed closure даже
# после retirement caller. Borrow endpoint/request/deadline; broker владеет mutable
# terminal state. published/completed — buffered outputs. Не обновлять/повторять work.
supervise(endpoint: ref Endpoint, request: ref Request, deadline: int,
          published: chan of int, completed: chan of Reply)
{
    published <-= control->pid();
    absent := empty();
    started := chan[1] of int;
    signal := chan[1] of int;
    spawn watch(endpoint, started, signal, deadline);
    watchdog := <-started;
    delivered := 0;
    result := Reply(Proxyowner->NotSent, 0, absent);
    if (!endpoint.state.closed && sys->millisec() < deadline) {
        alt {
            endpoint.requests <-= request => delivered = 1;
            <-signal => {}
        }
    }
    if (!delivered && endpoint.state.closed) {
        result.status = Disconnected;
    }
    if (delivered) {
        alt {
            result = <-request.reply => {}
            <-signal =>
                alt {
                    result = <-request.reply => {}
                    * => result = Reply(Proxyowner->Unknown, 0, absent);
                }
        }
    }
    control->killwait(watchdog);
    completed <-= result;
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
    deadline := sys->millisec() + timeout;
    response := chan[1] of Reply;
    request := ref Request(operation, target, left, right, refs, value, deadline, response);
    published := chan[1] of int;
    completed := chan[1] of Reply;
    spawn supervise(endpoint, request, deadline, published, completed);
    supervisor := <-published;
    result := <-completed;
    control->killwait(supervisor);
    return result;
}


# EN/RU: Run ordinary scalar operation; explicit Drop consumes this logical image once.
# Исполнить scalar operation; explicit Drop потребляет logical image один раз.
call(endpoint: ref Endpoint, operation: int, target, left, right: Image,
     refs, value, timeout: int): (int, int)
{
    if ((operation < Proxyowner->Read || operation > Proxyowner->Stop) && operation != Proxyowner->Add) {
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


# EN/RU: Typed callback result owns a new export; ordinary scalar call cannot fetch it.
# Typed callback result владеет new export; scalar call не принимает его.
fetch(endpoint: ref Endpoint, target, left, right: Image,
      refs, value, kind, timeout: int): (int, Image)
{
    if (kind != Proxyhost->Variable && kind != Proxyhost->Function) {
        return (Proxyowner->NotSent, empty());
    }
    result := offer(endpoint, Proxyowner->ReturnVariable + kind, target, left, right, refs, value, timeout);
    return (result.status, result.image);
}


# EN/RU: Reserve result ownership before importing an authorized server forwarding slot.
# Резервировать result ownership до import authorized server forwarding slot.
importref(endpoint: ref Endpoint, function: Image, slot, timeout: int): (int, Image)
{
    absent := empty();
    result := offer(endpoint, Proxyowner->ImportRoute, function, absent, absent, 0, slot, timeout);
    return (result.status, result.image);
}


# EN/RU: Only the remote registry may interpret forwarded ref handles for callbacks.
# Лишь remote registry интерпретирует forwarded ref handles для callbacks.
routed(endpoint: ref Endpoint, function, left, right: Image,
       refs, value, timeout: int): (int, int)
{
    result := offer(endpoint, Proxyowner->RoutedInvoke, function, left, right, refs, value, timeout);
    return (result.status, result.value);
}


# EN/RU: A routed variable result gets an independent reserved/bound image token.
# Routed variable result получает independent reserved/bound image token.
routedfetch(endpoint: ref Endpoint, function, left, right: Image,
            refs, value, timeout: int): (int, Image)
{
    result := offer(endpoint, Proxyowner->RoutedReturnVariable, function, left, right, refs, value, timeout);
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
            release := ref Request(Proxyowner->Drop, target, absent, absent, 0, 0, idle, nil);
            id++;
            (status, unused, terminal, ignored) := io->exchange(input, output, release, endpoint.high, endpoint.low, id, idle);
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
        if (request.deadline <= sys->millisec()) {
            admitted = 0;
            result.status = Proxyowner->NotSent;
        }
        if (admitted && request.operation == Proxyowner->Retain) {
            owner := request.target.owner;
            staged = gc->make(connection, owner.session, owner.slot, owner.generation, 0);
            if (staged == nil) {
                admitted = 0;
                result.status = Proxyhost->Capacity;
            }
        }
        resultcall := request.operation == Proxyowner->ReturnVariable || request.operation == Proxyowner->ReturnFunction || request.operation == Proxyowner->ImportRoute || request.operation == Proxyowner->RoutedReturnVariable;
        if (admitted && resultcall) {
            staged = gc->reserve(connection);
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
            returned := Proxyowner->Handle(big 0, 0, big 0);
            deadline := sys->millisec() + timeout;
            if (request.deadline < deadline) {
                deadline = request.deadline;
            }
            (result.status, result.value, closed, returned) = io->exchange(input, output, request,
                endpoint.high, endpoint.low, id, deadline);
            if (!closed && result.status == 0 && request.operation == Proxyowner->Retain) {
                if (!gc->activate(staged)) {
                    raise "proxy staged lease activation refused";
                }
                result.image = request.target;
                result.image.lease = staged;
            }
            if (!closed && result.status == 0 && resultcall) {
                if (!gc->bind(staged, returned.session, returned.slot, returned.generation)) {
                    raise "proxy reserved result binding refused";
                }
                result.image = Image(endpoint.high, endpoint.low, returned, staged);
            }
        }
        terminal := closed || (admitted && request.operation == Proxyowner->Stop);
        if (terminal) {
            gc->close(connection);
        }
        # EN/RU: Publish the active reply before closure wakes other queued callers.
        # Опубликовать active reply до closure, пробуждающего queued callers.
        request.reply <-= result;
        if (terminal) {
            endpoint.state.closed = 1;
        }
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
            codec->get(hello, Proxyframe->Status) == big 0 && codec->get(hello, Proxyframe->Result) == big 0 &&
            codec->get(hello, Proxyframe->ResultKind) == big 0 &&
            codec->get(hello, Proxyframe->Returned) == big 0 &&
            codec->get(hello, Proxyframe->Returned + 8) == big 0 &&
            codec->get(hello, Proxyframe->Returned + 16) == big 0) {
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
