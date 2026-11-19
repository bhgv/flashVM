# Copyright (c) 2026 bhgv
# EN: Retain a module's event FD across return/reentry and isolate its FORKNS from the caller's inbox.
# RU: Сохранить FD событий модуля через возврат/повторный вход и отделить его FORKNS от очереди вызывающего.
implement Value;
include "value.m";
include "sys.m";
inbox: ref Sys->FD; # Owned descriptor for this exact module instance's original inbox.
calls: int; # Instance-local invocation count; two loaded instances start independently.


# EN: First call opens/drains its inbox and forks; second call consumes the pending notice after caller-side work.
# Return the real dependency value on first call and 111 after reentry checks; raise on scope/FD loss.
# RU: Первый вызов открывает очередь и отделяет namespace; второй читает оставшееся событие после работы вызывающего.
# Вернуть значение зависимости при первом вызове, затем 111; при потере области или FD бросить исключение.
value(): int
{
    sys := load Sys Sys->PATH;
    bytes := array[16384] of byte;
    calls++;
    if (calls == 1) {
        inbox = sys->open("/dev/sysevents", Sys->OREAD);
        if (inbox == nil) {
            raise "module event grant missing";
        }
        while (sys->read(inbox, bytes, len bytes) > 0) {
        }
        if (sys->pctl(Sys->FORKNS, nil) < 0 || sys->bind("/data", "/moduleonly", 0) < 0) {
            raise "module event scope fork failed";
        }
        dependency := load Value "/dep.dis";
        if (dependency == nil) {
            return -1;
        }
        return dependency->value();
    }
    if (calls != 2 || sys->open("/moduleonly", Sys->OREAD) == nil ||
        sys->open("/calleronly", Sys->OREAD) != nil) {
        raise "module namespace reentry lost isolation";
    }
    if (sys->read(inbox, bytes, len bytes) <= 0) {
        raise "module inbox lost pending notice across return";
    }
    return 111;
}
