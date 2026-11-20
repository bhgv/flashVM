# Copyright (c) 2026 bhgv
# EN: Shared one-shot scalar service. Startup data selects the owner's implementation and Styx files.
# RU: Общий одноразовый скалярный сервис. Стартовые данные выбирают реализацию владельца и файлы Styx.
implement Scalarowner;
include "sys.m";
include "vfsnative.m";
include "launchcontext.m";

Scalarowner: module
{
    init: fn();
};


# EN: Read one admitted argument, invoke the ordinary linked Dis export and return its result.
# RU: Принять проверенный аргумент, вызвать обычный связанный экспорт Dis и вернуть результат.
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
    if (count != 6) {
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
    replytag := big hd tl fields;

    target := routes->scalarload(implementation, name);
    if (target == nil) {
        raise "scalar owner implementation link failed";
    }
    value := routes->scalartake(request, requesttag);
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
}
