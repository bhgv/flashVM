# Copyright (c) 2026 bhgv
# EN: Ordinary application receives terminal channels through a typed init argument.
# RU: Обычное приложение получает terminal channels типизированным аргументом init.
implement Terminalcmd;
include "terminalcmd.m";


# EN/RU: Raise a visible mismatch in both interpreter and JIT, never optimize away checks.
# Явно завершить mismatch в interpreter/JIT; проверки не оптимизируются прочь.
check(ok: int, message: string)
{
    if (!ok) {
        raise message;
    }
}


# EN/RU: Borrow session/args for one call; binary I/O is ordinary Sys, extra nodes use explicit root.
# Session/args borrowed на один вызов; binary I/O через Sys, extra nodes через explicit root.
init(terminal: ref Terminal->Session, args: list of string)
{
    sys := load Sys Sys->PATH;
    api := load Vfsexport Vfsexport->PATH;
    check(terminal != nil && terminal.receive != nil && terminal.send != nil && args != nil &&
        hd args == "on-provider", "typed init argument missing");
    payload := array[2048] of byte;
    for (i := 0; i < len payload; i++) {
        payload[i] = byte i;
    }
    check(sys->write(terminal.send, payload, len payload) == len payload, "terminal binary send");
    reply := array[len payload] of byte;
    check(sys->readn(terminal.receive, reply, len reply) == len reply, "fragmented terminal receive");
    for (i = 0; i < len reply; i++) {
        check(reply[i] == payload[i], "terminal payload changed");
    }
    check(api->open(terminal.root, "streams/17", Sys->ORDWR) == nil, "bulk visible before offer");
    check(sys->write(terminal.send, array of byte "offer", 5) == 5, "control offer");
    name := array[10] of byte;
    check(sys->readn(terminal.receive, name, len name) == len name && string name == "streams/17", "control reply");
    bulk := api->open(terminal.root, string name, Sys->ORDWR);
    check(bulk != nil && sys->write(bulk, payload, len payload) == len payload &&
        sys->readn(bulk, reply, len reply) == len reply, "additional bulk channel");
    check(sys->read(terminal.receive, reply, 1) == 0, "bulk mixed into control");
    sys->print("PASS typed terminal application init\n");
}
