# Copyright (c) 2026 bhgv
# EN: A child inherits launch requirements and rejects a changed target after the parent loaded the original implementation.
# RU: Child наследует launch requirements и отвергает changed target после загрузки original implementation родителем.
implement Check;
include "sys.m";
include "value.m";
Check: module
{
    init: fn(); # Existing console bootstrap ABI, no FlashVM context required.
};


# EN/RU: Load the changed target on this child's own permanent owner; report refusal, never execute its incompatible function.
# Загрузить changed target на постоянном owner child; сообщить refusal, не исполнять incompatible function.
child(done: chan of int)
{
    app := load Value "/app.dis";
    if (app != nil) {
        ignored := app->value();
        raise "child accepted changed launch target";
    }
    done <-= 1;
}


# EN/RU: Establish a valid initial load, change future provider bytes, then require the child's inherited contract to refuse them.
# Выполнить valid initial load, сменить future provider bytes и требовать refusal по inherited contract child.
init()
{
    sys := load Sys Sys->PATH;
    app := load Value "/app.dis";
    if (app == nil || app->value() != 111) {
        raise "original launch target unavailable";
    }
    control := sys->open("/switch", Sys->OWRITE);
    if (control == nil || sys->write(control, array of byte "x", 1) != 1) {
        raise "target interface switch refused";
    }
    done := chan of int;
    spawn child(done);
    if (<-done != 1) {
        raise "missing child interface refusal";
    }
    sys->print("PASS provider-backed Dis module and dependency\n");
}
