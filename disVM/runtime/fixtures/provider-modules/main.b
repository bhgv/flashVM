# Copyright (c) 2026 bhgv
# EN: Real native guest checks provider-first dependencies and terminal-first ordinary data without a display.
# RU: Настоящий native guest проверяет provider-first зависимости и terminal-first данные без дисплея.
implement Check;
include "sys.m";
include "value.m";
include "launchcontext.m";
Check: module
{
    init: fn(); # Existing zero-argument startup ABI.
};


HELPERS


# EN/RU: Exercise actual load/call and ordinary data FD; injected expected constants belong only to this test source.
# Проверить настоящий load/call и обычный data FD; подставляемые expected-константы относятся лишь к тесту.
init()
{
    sys := load Sys Sys->PATH;
    launch := load Launchcontext Launchcontext->PATH;
    if (launch == nil || launch->name() != "/app.dis") {
        raise "missing copied launch label";
    }
    arguments := launch->arguments();
    if (len arguments != 4 || int arguments[0] != 0 || int arguments[1] != 255 || int arguments[3] != 128) {
        raise "missing binary launch arguments";
    }
    app := load Value launch->name();
    if (app == nil || app->value() != EXPECTED) {
        raise "provider application dependency mismatch";
    }
    data := sys->open("/data", Sys->OREAD);
    bytes := array[32] of byte;
    n := sys->read(data, bytes, len bytes);
    if (n != 8 || string bytes[:n] != "terminal") {
        raise "ordinary data lost terminal-first order";
    }
    SWITCH
    sys->print("PASS provider-backed Dis module and dependency\n");
}
