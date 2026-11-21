# Copyright (c) 2026 bhgv
# EN: Shared sequential scalar service. Startup data selects the owner's implementation and Styx files.
# RU: Общий последовательный скалярный сервис. Стартовые данные выбирают реализацию владельца и файлы Styx.
implement Scalarowner;
include "sys.m";
include "vfsnative.m";
include "launchcontext.m";

Scalarowner: module
{
    init: fn();
};


# EN: Load once, then serve the configured sequence through the ordinary linked Dis export.
# RU: Загрузить один раз и обслужить последовательность обычным связанным экспортом Dis.
init()
{
    sys := load Sys Sys->PATH;
    context := load Launchcontext Launchcontext->PATH;
    routes := load Vfsnative Vfsnative->PATH;
    if (sys == nil || context == nil || routes == nil) {
        raise "scalar owner services unavailable";
    }
    startup := context->arguments();
    if (len startup == 0 || len startup > 4096) {
        raise "invalid scalar owner startup configuration";
    }
    (count, fields) := sys->tokenize(string startup, "\t");
    if (count == 5 && hd fields == "list") {
        serve_list(routes, tl fields);
        return;
    }
    if (count == 5 && hd fields == "ref") {
        serve_ref(routes, tl fields);
        return;
    }
    if (count == 5 && hd fields == "tuple") {
        serve_tuple(routes, tl fields);
        return;
    }
    if (count == 7 && hd fields == "big") {
        serve_big(routes, tl fields);
        return;
    }
    if (count > 0 && (hd fields == "set" || hd fields == "mixed")) {
        serve_set(sys, routes, count-1, tl fields, hd fields == "mixed");
        return;
    }
    if (count != 6 && count != 7) {
        raise "invalid scalar owner startup configuration";
    }
    implementation := hd fields;
    fields = tl fields;
    name := hd fields;
    fields = tl fields;
    request := hd fields;
    fields = tl fields;
    reply := hd fields;
    fields = tl fields;
    requesttag := big hd fields;
    fields = tl fields;
    replytag := big hd fields;
    calls := 1;
    if (count == 7) {
        calls = int hd tl fields;
    }
    if (calls < 0 || calls > 65536) {
        raise "invalid scalar owner call count";
    }

    target := routes->scalarload(implementation, name);
    if (target == nil) {
        raise "scalar owner implementation link failed";
    }
    remaining := calls;
    while (calls == 0 || remaining > 0) {
        value: int;
        if (calls == 0) {
            (ready, argument) := routes->scalarnext(request, requesttag);
            if (ready == 0) {
                return;
            }
            value = argument;
        } else {
            value = routes->scalartake(request, requesttag);
        }
        result := 0;
        success := 1;
        {
            result = target->invoke(value);
        } exception {
            "*" =>
                success = 0;
                result = 1;
        }
        routes->scalarreply(reply, replytag, success, result);
        if (calls > 0) {
            remaining--;
        }
    }
}


# EN/RU: Serve separate file routes on a single linked Dis data instance.
# Обслуживать отдельные файловые маршруты на одном экземпляре данных Dis.
serve_set(sys: Sys, routes: Vfsnative, count: int, fields: list of string, mixed: int)
{
    if (count < 4 || count > 25 || (count-1)%3 != 0) {
        raise "invalid owner export set";
    }
    implementation := hd fields;
    fields = tl fields;
    size := (count-1)/3;
    requests := array[size] of string;
    replies := array[size] of string;
    active := array[size] of int;
    tags := array[size] of big;
    names := hd fields;
    for (i := 0; i < size; i++) {
        if (i > 0) {
            names += "\t" + hd fields;
        }
        name := hd fields;
        tags[i] = big 72;
        if (mixed && len name > 2 && name[0] == 'b' && name[1] == ':') {
            tags[i] = big 76;
        }
        fields = tl fields;
        requests[i] = hd fields;
        fields = tl fields;
        replies[i] = hd fields;
        fields = tl fields;
        active[i] = 1;
    }
    target: ScalarimplementationSet;
    wide: ScalarbigimplementationSet;
    if (mixed) {
        (target, wide) = routes->scalarmixedload(implementation, names);
    } else {
        target = routes->scalarloadset(implementation, names);
    }
    if (target == nil || (mixed && wide == nil)) {
        raise "scalar owner export set link failed";
    }
    remaining := size;
    while (remaining > 0) {
        worked := 0;
        for (i = 0; i < size; i++) {
            if (active[i] == 0) {
                continue;
            }
            (ready, value) := routes->scalarbigpoll(requests[i], tags[i]);
            if (ready == 0) {
                active[i] = 0;
                remaining--;
            } else if (ready > 0) {
                result := big 0;
                success := 1;
                {
                    if (tags[i] == big 76) {
                        result = invoke_big(wide, i, value);
                    } else {
                        result = big invoke(target, i, int value);
                    }
                } exception {
                    "*" =>
                        success = 0;
                        result = big 1;
                }
                routes->scalarbigreply(replies[i], big 73, success, result);
                worked = 1;
            }
        }
        if (worked == 0 && remaining > 0) {
            sys->sleep(10);
        }
    }
}


# EN/RU: Route the validated ordinal through normal Dis module calls.
# Передать проверенный номер через обычные вызовы модуля Dis.
invoke(target: ScalarimplementationSet, slot, value: int): int
{
    case slot {
    0 => return target->invoke0(value);
    1 => return target->invoke1(value);
    2 => return target->invoke2(value);
    3 => return target->invoke3(value);
    4 => return target->invoke4(value);
    5 => return target->invoke5(value);
    6 => return target->invoke6(value);
    7 => return target->invoke7(value);
    }
    raise "invalid owner export slot";
}



# EN: Dispatch full-width values through the corresponding typed view of the same link.
# RU: Передать полноразрядное значение через типизированное представление той же связи.
invoke_big(target: ScalarbigimplementationSet, slot: int, value: big): big
{
    case slot {
    0 => return target->invoke0(value);
    1 => return target->invoke1(value);
    2 => return target->invoke2(value);
    3 => return target->invoke3(value);
    4 => return target->invoke4(value);
    5 => return target->invoke5(value);
    6 => return target->invoke6(value);
    7 => return target->invoke7(value);
    }
    raise "invalid wide owner export slot";
}


# EN: Serve a full-width export using the same native mailbox and ordinary Dis link.
# RU: Обслужить полноразрядный экспорт через тот же приёмник и обычную связь Dis.
serve_big(routes: Vfsnative, fields: list of string)
{
    implementation := hd fields;
    fields = tl fields;
    name := hd fields;
    fields = tl fields;
    request := hd fields;
    fields = tl fields;
    reply := hd fields;
    fields = tl fields;
    requesttag := big hd fields;
    replytag := big hd tl fields;
    target := routes->scalarbigload(implementation, name);
    if (target == nil) {
        raise "big owner implementation link failed";
    }
    for (;;) {
        (ready, value) := routes->scalarbignext(request, requesttag);
        if (ready == 0) {
            return;
        }
        result := big 0;
        success := 1;
        {
            result = target->invoke(value);
        } exception {
            "*" =>
                success = 0;
                result = big 1;
        }
        routes->scalarbigreply(reply, replytag, success, result);
    }
}



# EN: Serve tuple requests through a single linked original Dis instance.
# RU: Обслуживать запросы кортежей одним связанным экземпляром исходного Dis.
serve_tuple(routes: Vfsnative, fields: list of string)
{
    implementation := hd fields;
    fields = tl fields;
    name := hd fields;
    fields = tl fields;
    request := hd fields;
    reply := hd tl fields;
    target := routes->scalartupleload(implementation, name);
    if (target == nil) {
        raise "tuple owner implementation link failed";
    }
    for (;;) {
        (ready, narrow, wide) := routes->scalartuplenext(request);
        if (ready == 0) {
            return;
        }
        result := (0, big 0);
        success := 1;
        {
            result = target->invoke((narrow, wide));
        } exception {
            "*" =>
                success = 0;
        }
        routes->scalartuplereply(reply, success, result);
    }
}



# EN: Link the configured export once and pass one temporary managed cell per request.
# RU: Связать настроенный экспорт один раз и выделить временный объект на каждый запрос.
serve_ref(routes: Vfsnative, fields: list of string)
{
    implementation := hd fields;
    fields = tl fields;
    name := hd fields;
    fields = tl fields;
    request := hd fields;
    reply := hd tl fields;
    target := routes->scalarrefload(implementation, name);
    if (target == nil) {
        raise "ref owner implementation link failed";
    }
    for (;;) {
        (ready, narrow, wide) := routes->scalarrefnext(request);
        if (ready == 0) {
            return;
        }
        cell := ref Scalarrefcell(narrow, wide);
        result := (0, big 0);
        success := 1;
        {
            result = target->invoke(cell);
        } exception {
            "*" =>
                success = 0;
        }
        routes->scalarrefreply(reply, success, result, (cell.narrow, cell.wide));
    }
}



# EN: Invoke the verified multi-argument export on one linked Dis instance.
# RU: Вызвать проверенный экспорт с несколькими аргументами в одном экземпляре Dis.
serve_list(routes: Vfsnative, fields: list of string)
{
    implementation := hd fields;
    fields = tl fields;
    name := hd fields;
    fields = tl fields;
    request := hd fields;
    reply := hd tl fields;
    target := routes->scalarlistload(implementation, name);
    if (target == nil) {
        raise "argument-list owner implementation link failed";
    }
    for (;;) {
        (ready, seed, narrow, wide, delta) := routes->scalarlistnext(request);
        if (ready == 0) {
            return;
        }
        outer := inner := 0;
        value := big 0;
        success := 1;
        {
            (outer, (inner, value)) = target->invoke(seed, (narrow, wide), delta);
        } exception {
            "*" =>
                success = 0;
        }
        routes->scalarlistreply(reply, success, outer, inner, value);
    }
}
