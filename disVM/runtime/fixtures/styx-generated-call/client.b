# Copyright (c) 2026 bhgv
# EN: Call the generated body with mixed Dis arguments.
# RU: Вызвать сгенерированное тело со смешанными аргументами Dis.
implement Client;
include "sys.m";
Target: module {
    invoke: fn(x: byte, s: string, a: array of byte, n: big): string;
};
Client: module {
    init: fn();
    go: fn(m: Target, x: byte, s: string, a: array of byte, n: big): string;
};

# EN: Pass the checked typed frame to the separate module.
# RU: Передать типизированный кадр отдельному модулю.
go(m: Target, x: byte, s: string, a: array of byte, n: big): string
{
    return m->invoke(x, s, a, n);
}

# EN: Assert that the original export returns a managed string.
# RU: Проверить управляемую строку из исходного экспорта.
init()
{
    sys := load Sys Sys->PATH;
    target := load Target "./target.dis";
    if (target == nil || go(target, byte 23, "Ж", array[] of {byte 11}, big 77) != "Ж!") {
        raise "synthesized call result mismatch";
    }
    sys->print("PASS synthesized Dis call executed\n");
}
