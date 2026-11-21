# Copyright (c) 2026 bhgv
# EN: Load two libraries on one ordinary Styx mount and compare their executor IDs.
# RU: Загрузить две библиотеки с одного маунта Styx и сравнить PID исполнителей.
implement Librarycli;

include "sys.m";
include "scoped-calls.m";

Librarycli: module
{
    init: fn();
};

# EN: Test ordinary load, scoped ref copy-back and one executor across modules.
# RU: Проверить обычный load, обратную запись ref и общего исполнителя модулей.
init()
{
    sys := load Sys Sys->PATH;
    if (sys->bind("/transport/mounted", "/libraries", Sys->MAFTER) < 0) {
        raise "could not add the Styx library directory to the local union: " + sys->sprint("%r");
    }
    if (sys->open("/libraries/local-marker", Sys->OREAD) == nil) {
        raise "local member of the library union disappeared";
    }
    first := load Scopedcalls "/libraries/first.dis";
    second := load Scopedcalls "/libraries/second.dis";
    if (first == nil || second == nil) {
        raise "could not load both mounted libraries: " + sys->sprint("%r");
    }
    pid := sys->pctl(0, nil);
    a := ref Cell(-51, big 0);
    b := ref Cell(-51, big 0);
    if (first->update(a) != pid || second->update(b) != pid ||
        a.narrow != pid || b.narrow != pid || a.wide <= big 0 || a.wide != b.wide) {
        raise "mounted libraries did not share the expected executor";
    }
    sys->print("PASS two libraries on one stock Styx session\n");
}
