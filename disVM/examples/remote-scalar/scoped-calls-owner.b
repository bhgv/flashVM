# Copyright (c) 2026 bhgv
# EN: Read the caller's mounted context through ordinary Sys operations.
# RU: Читать смонтированный контекст клиента обычными операциями Sys.
implement Scopedcalls;
include "sys.m";
include "vfsnative.m";
include "scoped-calls.m";

Caller: con "/.prog/.self/"; # Alias set in the private namespace of one call.
saved_reverse: ref Sys->FD; # Caller stream retained across reply to test revocation.
saved_events: ref Sys->FD; # Event FD retained for a post-reply retirement check.
saved: ref Sys->FD; # An unread caller fid from the first completed inspection.


# EN: Read at most limit bytes from a single context file or report owner failure.
# RU: Прочитать не больше limit байт файла контекста или сообщить об ошибке.
contents(sys: Sys, path: string, limit: int): string
{
    handle := sys->open(path, Sys->OREAD);
    if (handle == nil) {
        raise "caller file unavailable: " + path;
    }
    data := array[limit] of byte;
    n := sys->read(handle, data, len data);
    if (n <= 0) {
        raise "caller read failed: " + path;
    }
    return string data[0:n];
}


# EN: Verify the selected PID, grants and inherited FD; return its four bytes as a code.
# RU: Сверить выбранный PID, выдачи и унаследованный FD; вернуть код четырёх байтов.
inspect(fail: int): (int,big)
{
    sys := load Sys Sys->PATH;
    if (sys->pctl(Sys->FORKNS, nil) <= 0) {
        raise "failed to copy scoped namespace";
    }
    reverse := sys->open(Caller + ".sysevents", Sys->OREAD);
    event := array[4096] of byte;
    if (reverse == nil || !namespace_event(sys, reverse, event) ||
        sys->open(Caller + ".sysevents", Sys->OWRITE) != nil) {
        raise "reverse event stream missing or writable";
    }
    pid := int contents(sys, Caller + ".pid", 32);
    if (pid <= 0 || contents(sys, Caller + ".vfs/env/CLIENT", 8) != "/.vfs" ||
        contents(sys, Caller + ".var/counter", 8) != "/.var") {
        raise "wrong caller scope";
    }
    if (fail == 2 || fail == 3) {
        wait_cancel(sys, fail == 3);
        raise "remote handler stopped at a cancellation checkpoint";
    }
    if (fail == 6) {
        edit_vector(sys);
        return (pid, big 0);
    }
    if (fail == 5) {
        return (pid, big sys->pctl(0, nil));
    }
    if (fail == 4) {
        sys->sleep(3500);
        return (pid, big 44);
    }
    if (fail != 0) {
        raise "requested scoped owner failure";
    }
    value := contents(sys, Caller + ".fd/3", 4);
    if (value == "left") {
        initial_grants(sys, reverse, event);
        if (sys->read(reverse, event, len event) != 0) {
            raise "idle reverse queue did not report pending";
        }
        if (sys->open(Caller + ".vfs/private-live", Sys->OREAD) != nil) {
            raise "unshared variable visible through caller VFS";
        }
        set_marker(sys, "refpair-entered");
        wait_marker(sys, "refpair-edited");
        variables := 0;
        filesystem := 0;
        for (i := 0; i < 2; i++) {
            n := sys->read(reverse, event, len event);
            if (n < 22) {
                break;
            }
            if (string event[0:11] == "system 1 4 ") {
                path := string event[n-11:n];
                if (path == "2f2e766172\n") {
                    variables++;
                } else if (path == "2f2e766673\n") {
                    filesystem++;
                }
            }
        }
        if (variables != 1 || filesystem != 1 ||
            !live_value(sys, Caller + ".var/live", 1, 3) ||
            !live_value(sys, Caller + ".vfs/live", 2, 5)) {
            raise "did not receive both live caller events and values";
        }
        if (sys->read(reverse, event, len event) != 0) {
            raise "unexpected record after both granted notifications";
        }
        check_overflow(sys, reverse, event);
        saved_reverse = reverse;
        saved = sys->open(Caller + ".pid", Sys->OREAD);
        if (saved == nil) {
            raise "could not save the scoped fid";
        }
        return (pid, big 1);
    }
    if (value == "next") {
        return (pid, big 2);
    }
    if (value == "last") {
        return (pid, big 3);
    }
    raise "wrong inherited caller fd data";
}


# EN: Apply an update sourced from the caller's actual scoped identity.
# RU: Применить изменение из фактического PID в контексте клиента.
update(value: ref Cell): int
{
    sys := load Sys Sys->PATH;
    pid := int contents(sys, Caller + ".pid", 32);
    if (value.narrow == -51) {
        value.wide = big sys->pctl(0, nil);
    }
    if (value.narrow == -50) {
        pause_ref(sys, "single-ref");
    }
    value.narrow = pid;
    return pid;
}


# EN: Check a private event inbox before and after detaching the call namespace.
# RU: Проверить частную очередь событий до и после отделения namespace вызова.
updatepair(values: (ref Cell, ref Cell))
{
    sys := load Sys Sys->PATH;
    (first, second) := values;
    if (first.narrow == -51) {
        first.wide = big sys->pctl(0, nil);
        return;
    }
    if (first.narrow == -50) {
        pause_ref(sys, "pair-ref");
    }
    fail := first.narrow < 0;
    inbox := sys->open("/dev/sysevents", Sys->OREAD);
    bytes := array[16384] of byte;
    if (inbox == nil || !namespace_event(sys, inbox, bytes)) {
        raise "call event inbox unavailable";
    }
    if (sys->pctl(Sys->FORKNS, nil) <= 0 || !namespace_event(sys, inbox, bytes)) {
        raise "call event inbox did not follow forked namespace";
    }
    pid := int contents(sys, Caller + ".pid", 32);
    first.narrow = pid;
    second.narrow += 2;
    saved_events = inbox;
    if (fail) {
        raise "requested scoped ref failure";
    }
}


# EN: Look for a leaked caller alias from a control call with no scope.
# RU: Проверить отсутствие утечки алиаса в контрольном вызове без контекста.
baseline(flag: int): int
{
    sys := load Sys Sys->PATH;
    if (sys->open(Caller + ".pid", Sys->OREAD) != nil) {
        raise "caller scope leaked into unscoped export";
    }
    if (saved != nil) {
        data := array[32] of byte;
        if (sys->read(saved, data, len data) > 0) {
            raise "saved caller fid remained readable after the reply";
        }
        saved = nil;
    }
    if (saved_reverse != nil) {
        data := array[4096] of byte;
        if (sys->read(saved_reverse, data, len data) >= 0) {
            raise "saved reverse event fid survived the reply";
        }
        saved_reverse = nil;
    }
    if (saved_events != nil) {
        retired_events(sys, saved_events);
        saved_events = nil;
    }
    return flag;
}


# EN: Require a namespace notification on the inbox selected by this call.
# RU: Проверить уведомление namespace из очереди этого вызова.
namespace_event(sys: Sys, inbox: ref Sys->FD, bytes: array of byte): int
{
    n := sys->read(inbox, bytes, len bytes);
    return n >= 9 && string bytes[0:9] == "system 2 ";
}


# EN: Drain a saved call inbox, validating its retirement event and EOF.
# RU: Дочитать сохранённую очередь вызова и проверить завершение и EOF.
retired_events(sys: Sys, inbox: ref Sys->FD)
{
    bytes := array[16384] of byte;
    exited := 0;
    for (i := 0; i < 34; i++) {
        n := sys->read(inbox, bytes, len bytes);
        if (n == 0 && exited) {
            return;
        }
        if (n < 9) {
            break;
        }
        if (string bytes[0:9] == "system 4 ") {
            exited = 1;
        }
    }
    raise "saved call event inbox did not retire";
}

include "refpair-sync.b";

# EN: Check the updated four-byte big-endian value in the named caller file.
# RU: Проверить новое четырёхбайтовое значение в указанном файле клиента.
live_value(sys: Sys, path: string, high, low: int): int
{
    live := sys->open(path, Sys->OREAD);
    encoded := array[4] of byte;
    return live != nil && sys->read(live, encoded, 4) == 4 &&
        encoded[0] == byte 0 && encoded[1] == byte high &&
        encoded[2] == byte 0 && encoded[3] == byte low;
}

# EN: Drain the two initial grant snapshots before testing an idle read.
# RU: Прочитать два начальных снимка выдач перед проверкой пустой очереди.
initial_grants(sys: Sys, inbox: ref Sys->FD, bytes: array of byte)
{
    for (i := 0; i < 2; i++) {
        n := sys->read(inbox, bytes, len bytes);
        if (n < 12 || string bytes[0:12] != "system 1 63 ") {
            raise "missing initial grant snapshot";
        }
    }
}

# EN: Verify overflow, authoritative reread and a subsequent notice on this same fid.
# RU: Проверить переполнение, повторное чтение значения и следующее уведомление на том же fid.
check_overflow(sys: Sys, reverse: ref Sys->FD, event: array of byte)
{
    set_marker(sys, "overflow-ready");
    wait_marker(sys, "overflow-done");
    n := sys->read(reverse, event, len event);
    if (n < 9 || string event[0:9] != "system 3 " ||
        !live_value(sys, Caller + ".var/live", 1, 33)) {
        raise "reverse event overflow did not resync";
    }
    n = sys->read(reverse, event, len event);
    if (n < 22 || string event[0:11] != "system 1 4 " ||
        string event[n-11:n] != "2f2e766172\n" ||
        sys->read(reverse, event, len event) != 0) {
        raise "overflow did not preserve the triggering notice";
    }
    set_marker(sys, "recovery-ready");
    wait_marker(sys, "recovery-done");
    n = sys->read(reverse, event, len event);
    if (n < 22 || string event[0:11] != "system 1 4 " ||
        string event[n-11:n] != "2f2e766172\n" ||
        !live_value(sys, Caller + ".var/live", 1, 99)) {
        raise "reverse event stream failed to recover";
    }
}

# EN: Observe revocation after entering the actual Dis function.
# RU: Увидеть отзыв fid уже после входа в функцию Dis.
wait_cancel(sys: Sys, compute: int)
{
    prefix := "cancel-";
    if (compute) {
        prefix = "compute-cancel-";
    }
    set_marker(sys, prefix + "entered");
    {
        if (compute) {
            for (i := 0; i < 2147483647; i++) {
                ;
            }
        } else {
            sys->sleep(30000);
        }
        raise "remote call cancellation timed out";
    } exception {
        "remote call cancelled" =>
            set_marker(sys, prefix + "observed");
    }
}

# EN: Keep a ref invocation active until the VM delivers cancellation.
# RU: Удерживать вызов ref до получения отмены от VM.
pause_ref(sys: Sys, prefix: string)
{
    set_marker(sys, prefix + "-pending");
    {
        sys->sleep(30000);
        raise "ref cancellation timed out";
    } exception {
        "remote call cancelled" =>
            set_marker(sys, prefix + "-cancelled");
            raise "remote call cancelled";
    }
}

# EN: Read collection metadata and update one byte-encoded member via reverse Styx.
# RU: Прочитать метаданные и изменить один элемент через обратный Styx.
edit_vector(sys: Sys)
{
    path := Caller + ".var/vector/";
    if (contents(sys, path + ".type", 24) != "array of int\n" ||
        contents(sys, path + ".len", 8) != "3\n" ||
        sys->open(path + ".len", Sys->OWRITE) != nil ||
        !live_value(sys, path + "0", 0, 7) || !live_value(sys, path + "1", 0, 9)) {
        raise "invalid live array directory";
    }
    member := sys->open(path + "0", Sys->OWRITE);
    bytes := array[] of {byte 0, byte 0, byte 0, byte 19};
    if (member == nil || sys->write(member, bytes, len bytes) != len bytes ||
        !live_value(sys, path + "2", 0, 19)) {
        raise "live array update lost its aliased cell";
    }
}
