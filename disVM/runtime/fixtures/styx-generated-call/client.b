# Copyright (c) 2026 bhgv
# EN: Call the generated body with mixed Dis arguments.
# RU: Вызвать сгенерированное тело со смешанными аргументами Dis.
implement Client;
include "sys.m";
Target: module {
    Cell: adt { narrow: int; wide: big; };
    update: fn(a, b: ref Cell): big;
    invoke: fn(x: byte, s: string, a: array of byte, n: big): string;
};
Gateway: module {
    invoke: fn(m: Target, x: byte, s: string, a: array of byte, n: big): string;
};
Refgateway: module {
    invoke: fn(m: Target, a, b: ref Target->Cell): big;
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
    gateway := load Gateway "./generated-gateway.dis";
    if (gateway == nil || gateway->invoke(target, byte 23, "Ж", array[] of {byte 11}, big 77) != "Ж!") {
        raise "synthesized module load result mismatch";
    }
    refs := load Refgateway "./generated-ref-gateway.dis";
    cell := ref Target->Cell(7, big 19);
    if (refs == nil || refs->invoke(target, cell, cell) != big 27 ||
        cell.narrow != 8 || cell.wide != big 27) {
        raise "synthesized call lost ref alias";
    }
    sys->print("PASS loaded synthesized Dis module executed\n");
    sys->print("PASS synthesized Dis call executed\n");
}
