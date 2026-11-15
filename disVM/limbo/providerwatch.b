# Copyright (c) 2026 bhgv
# EN: Pure Limbo event-file client; does not publish state or grant resource access.
# RU: Pure Limbo event-file client; не публикует state и не даёт resource access.
implement Providerwatch;
include "sys.m";
include "providerwatch.m";
sys: Sys; # Immutable builtin reference after init.


# EN/RU: Load Sys before callers share this module instance.
# Загрузить Sys до передачи module instance другим callers.
init()
{
    sys = load Sys Sys->PATH;
    if (sys == nil) {
        raise "provider watch requires Sys";
    }
}


# EN/RU: Require a fixed-width lower-case opaque hex token, including zero.
# Проверить fixed-width lowercase hex token, включая zero. No numeric narrowing.
hex(value: string): int
{
    if (len value != 16) {
        return 0;
    }
    for (i := 0; i < len value; i++) {
        c := value[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            return 0;
        }
    }
    return 1;
}


# EN/RU: Parse a bounded unsigned decimal mask; invalid returns-1.
# Разобрать bounded unsigned decimal mask; invalid возвращает-1.
number(value: string): int
{
    if (len value < 1 || len value > 2) {
        return -1;
    }
    result := 0;
    for (i := 0; i < len value; i++) {
        if (value[i] < '0' || value[i] > '9') {
            return -1;
        }
        result = result * 10 + value[i] - '0';
    }
    return result;
}


# EN/RU: Atomically change selected classes; unacked categories remain pending.
# Atomic смена classes; неподтверждённые категории остаются pending.
mask(watch: ref Watch, kinds: int): int
{
    if (watch == nil || watch.fd == nil || kinds < 1 || kinds > All) {
        return Error;
    }
    command := array of byte ("mask " + string kinds + "\n");
    if (sys->write(watch.fd, command, len command) != len command) {
        return Error;
    }
    watch.pending = nil;
    return 0;
}


# EN/RU: Own an ordinary descriptor; failed setup lets managed FD retire normally.
# Владеть ordinary FD; failed setup освобождает managed FD обычным путём.
open(path: string, kinds: int): (int, ref Watch)
{
    if (kinds < 1 || kinds > All) {
        return (Error, nil);
    }
    fd := sys->open(path, Sys->ORDWR);
    if (fd == nil) {
        return (Error, nil);
    }
    watch := ref Watch(fd, nil);
    (status, nil) := next(watch);
    if (status != EventReady) {
        return (Error, nil);
    }
    if (mask(watch, kinds) != 0) {
        return (Error, nil);
    }
    return (0, watch);
}


# EN/RU: Read one bounded frame; a malformed/partial source never advances ack.
# Прочитать bounded frame; malformed/partial source не продвигает ack.
next(watch: ref Watch): (int, ref Event)
{
    if (watch == nil || watch.fd == nil) {
        return (Error, nil);
    }
    bytes := array[128] of byte;
    count := sys->read(watch.fd, bytes, len bytes);
    if (count < 0 && sys->sprint("%r") == "native input temporarily unavailable") {
        return (Idle, nil);
    }
    if (count <= 0 || int bytes[count - 1] != '\n') {
        return (Error, nil);
    }
    (n, words) := sys->tokenize(string bytes[:count], " \n");
    if (n != 7 || hd words != "event") {
        return (Error, nil);
    }
    words = tl words;
    ticket := hd words;
    words = tl words;
    session := hd words;
    words = tl words;
    generation := hd words;
    words = tl words;
    kinds := number(hd words);
    words = tl words;
    flags := number(hd words);
    words = tl words;
    resource := hd words;
    if (!hex(ticket) || !hex(session) || !hex(generation) || !hex(resource) ||
        kinds < 1 || kinds > All || flags < 0 || flags > Resource) {
        return (Error, nil);
    }
    watch.pending = ticket;
    return (EventReady, ref Event(ticket, session, generation, kinds, flags, resource));
}


# EN/RU: Write only a delivered watch-local token; native endpoint validates it again.
# Записать только delivered watch-local token; native endpoint проверяет повторно.
ack(watch: ref Watch, event: ref Event): int
{
    if (watch == nil || watch.fd == nil || event == nil || watch.pending != event.ticket || !hex(event.ticket)) {
        return Error;
    }
    command := array of byte ("ack " + event.ticket + "\n");
    if (sys->write(watch.fd, command, len command) != len command) {
        return Error;
    }
    watch.pending = nil;
    return 0;
}
