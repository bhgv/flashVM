# Copyright (c) 2026 bhgv
# EN: Native stub authority resolves only to registered live source actors.
# RU: Native stub authority разрешается лишь в registered live source actors.
implement Proxyregistry;
include "sys.m";
include "proxyhost.m";
include "proxyref.m";
include "proxyhandler.m";
include "proxyowner.m";
include "proxynet.m";
include "proxyroute.m";
include "proxyregistry.m";
sys: Sys; # Immutable monotonic clock.
proxy: Proxyowner; # Original function owner and native stub lifetime.
route: Proxyroute; # Independent source image actors.
access: Proxyref; # Variable-only access; no foreign Cell dereference.
gc: Proxygc; # Exact token activity query before borrowing a registered ref.


# EN/RU: Initialize dependencies once before registry contexts are shared.
# Инициализировать dependencies один раз до sharing registry contexts.
init()
{
    sys = load Sys Sys->PATH;
    proxy = load Proxyowner Proxyowner->PATH;
    route = load Proxyroute Proxyroute->PATH;
    access = load Proxyref Proxyref->PATH;
    gc = load Proxygc Proxygc->PATH;
    if (sys == nil || proxy == nil || route == nil || access == nil || gc == nil) {
        raise "proxy registry modules unavailable";
    }
    proxy->init();
    route->init();
    access->init();
}


# EN/RU: Configured sources remain mapped owners; unknown destinations are never opened.
# Configured sources остаются mapped owners; unknown destinations не открываются.
create(sources: array of ref Source, endpoint: ref Proxyowner->Endpoint, timeout: int): ref Context
{
    if (sources == nil || len sources < 1 || len sources > 16 || endpoint == nil || timeout < 1 || timeout > 60000) {
        return nil;
    }
    snapshot := array[len sources] of ref Source;
    for (index := 0; index < len sources; index++) {
        if (sources[index] == nil || sources[index].endpoint == nil || sources[index].image.lease == nil) {
            return nil;
        }
        snapshot[index] = ref Source(sources[index].endpoint, sources[index].image);
    }
    return ref Context(snapshot, array[16] of ref View, array[256] of chan of int, 0, endpoint, timeout, 0);
}


# EN/RU: Exact session/slot/generation match; stale/reused native handles never select a view.
# Exact session/slot/generation match; stale/reused native handles не выбирают view.
lookup(context: ref Context, owner: Proxyowner->Handle): ref View
{
    if (context != nil && !context.closed) {
        for (index := 0; index < len context.views; index++) {
            view := context.views[index];
            if (view != nil && view.owners > 0 && view.owner.session == owner.session &&
                view.owner.slot == owner.slot && view.owner.generation == owner.generation) {
                return view;
            }
        }
    }
    return nil;
}


# EN/RU: Spawned actor's buffered completion survives import caller retirement.
# Buffered completion spawned actor переживает retirement import caller.
actor(source: ref Source, timeout, deadline: int, ready: chan of (int, ref Proxyref->Ref), done: chan of int)
{
    route->startuntil(source.endpoint, source.image, timeout, deadline, ready);
    done <-= 1;
}


# EN: Create independent source ownership, then publish a native variable stub
# under an admitted function pin. A failed import never returns a usable view.
# RU: Создать independent source ownership, затем native variable stub под
# admitted function pin. Failed import не возвращает usable view.
importview(context: ref Context, function: Proxyowner->Handle, slot, deadline: int): (int, Proxyowner->Handle)
{
    absent := Proxyowner->Handle(big 0, 0, big 0);
    if (context == nil || context.closed || slot < 0 || slot >= len context.sources) {
        return (3, absent);
    }
    empty := -1;
    for (index := 0; index < len context.views; index++) {
        if (context.views[index] == nil || context.views[index].owners == 0) {
            empty = index;
            break;
        }
    }
    if (empty < 0 || context.issued == len context.actors) {
        return (Proxyhost->Capacity, absent);
    }

    ready := chan[1] of (int, ref Proxyref->Ref);
    done := chan[1] of int;
    context.actors[context.issued++] = done;
    spawn actor(context.sources[slot], context.timeout, deadline, ready, done);
    (status, source) := <-ready;
    if (status != 0) {
        return (status, absent);
    }
    remaining := deadline - sys->millisec();
    if (remaining <= 0) {
        source = nil;
        return (Proxyowner->Unknown, absent);
    }
    (published, owner) := proxy->exposeuntil(context.endpoint, function, ref Proxyhost->Cell(empty), deadline);
    if (published != 0) {
        source = nil;
        return (published, absent);
    }
    context.views[empty] = ref View(owner, source, 1);
    return (0, owner);
}


# EN: Forward only registered views; ordinary Invoke/fetch must never receive a
# native stub as a copied local Cell. Count native Retain/Drop only after success.
# RU: Forward лишь registered views; ordinary Invoke/fetch не получает native stub
# как copied local Cell. Считать native Retain/Drop лишь после success.
dispatch(context: ref Context, operation: int, target, left, right: Proxyowner->Handle,
         refs, value, deadline: int): (int, int, int, Proxyowner->Handle, int)
{
    absent := Proxyowner->Handle(big 0, 0, big 0);
    if (operation == Proxyowner->ImportRoute) {
        if (refs != 0) {
            return (1, 3, 0, absent, 0);
        }
        (status, owner) := importview(context, target, value, deadline);
        if (status == Proxyref->Disconnected) {
            status = Proxyowner->Unknown;
        }
        kind := 0;
        if (status == 0) {
            kind = 1;
        }
        return (1, status, 0, owner, kind);
    }
    first := lookup(context, left);
    second := lookup(context, right);
    if (operation == Proxyowner->RoutedInvoke) {
        if (context == nil || context.closed ||
            (refs > 0 && (first == nil || first.source == nil || !gc->active(first.source.lease))) ||
            (refs > 1 && (second == nil || second.source == nil || !gc->active(second.source.lease)))) {
            return (1, Proxyref->Stale, 0, absent, 0);
        }
        lref, rref: ref Proxyref->Ref;
        if (refs > 0) {
            lref = first.source;
        }
        if (refs > 1) {
            rref = second.source;
        }
        (status, result) := proxy->routeduntil(context.endpoint, target, lref, rref, refs, value, deadline);
        return (1, status, result, absent, 0);
    }
    if ((operation == Proxyowner->Invoke || operation == Proxyowner->ReturnVariable || operation == Proxyowner->ReturnFunction) &&
        ((refs > 0 && first != nil) || (refs > 1 && second != nil))) {
        return (1, 3, 0, absent, 0);
    }
    view := lookup(context, target);
    if (view == nil) {
        return (0, 3, 0, absent, 0);
    }
    if (refs != 0) {
        return (1, 3, 0, absent, 0);
    }
    status := 3;
    result := 0;
    timeout := deadline - sys->millisec();
    if (timeout < 1) {
        return (1, Proxyowner->NotSent, 0, absent, 0);
    }
    if (operation == Proxyowner->Retain || operation == Proxyowner->Drop) {
        if (operation == Proxyowner->Retain && view.owners >= 256) {
            return (1, Proxyhost->Capacity, 0, absent, 0);
        }
        (status, result) = proxy->call(context.endpoint, operation, target, absent, absent, 0, 0, timeout);
        if (status == 0) {
            if (operation == Proxyowner->Retain) {
                view.owners++;
            } else if (--view.owners == 0) {
                view.source = nil;
            }
        }
    } else if (operation == Proxyowner->Read || operation == Proxyowner->Write || operation == Proxyowner->Add) {
        primitive := Proxyref->Read;
        if (operation == Proxyowner->Write) {
            primitive = Proxyref->Write;
        } else if (operation == Proxyowner->Add) {
            primitive = Proxyref->Add;
        }
        (status, result) = access->call(view.source, primitive, value, deadline);
        if (status == Proxyref->Disconnected) {
            status = Proxyowner->Unknown;
        }
    }
    return (1, status, result, absent, 0);
}


# EN: Kill/join the function owner first, then revoke all refs and join independent
# actors. Completion channels never contain a Ref, so they do not prevent its GC.
# RU: Сначала kill/join function owner, затем отозвать refs и join independent
# actors. Completion channels не содержат Ref и не мешают GC.
close(context: ref Context)
{
    if (context == nil || context.closed) {
        return;
    }
    context.closed = 1;
    for (index := 0; index < len context.views; index++) {
        view := context.views[index];
        if (view != nil && view.source != nil) {
            access->call(view.source, Proxyref->Close, 0, sys->millisec() + context.timeout);
            view.source = nil;
        }
    }
    for (index = 0; index < context.issued; index++) {
        <-context.actors[index];
    }
}
