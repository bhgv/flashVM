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
