# Copyright (c) 2026 bhgv
# EN: Assemble one immutable native stack ticket into private Limbo display data.
# RU: Собрать immutable native stack ticket в private Limbo display data.
implement Residentstack;
include "sys.m";
include "processctl.m";
include "residentstack.m";


# EN/RU: Copy fixed frame label bytes; discard partial string on target/ticket loss.
# Копировать fixed frame label bytes; отбросить partial string при target/ticket loss.
label(ctl: Processctl, id, ticket: big, frame, part, length: int): (int, string)
{
    bytes := array[length] of byte;
    for (index := 0; index < length; index++) {
        (status, value) := ctl->residentframebyte(id, ticket, frame, part, index);
        if (status != ctl->DiagReady) {
            return (status, nil);
        }
        bytes[index] = byte value;
    }
    return (ctl->DiagReady, string bytes);
}


# EN: Copy one summary's entire bounded prefix; each frame must have the same
# sequence. ctl/id/ticket are borrowed. No partial result escapes on failure.
# RU: Копировать весь bounded prefix summary; каждый frame с той же sequence.
# ctl/id/ticket заимствуются. Partial result не выходит при отказе.
copy(ctl: Processctl, id, ticket, sequence: big, count, flags: int): (int, ref Stack)
{
    if (count < 0 || count > 8) {
        return (ctl->DiagInvalid, nil);
    }
    frames := array[count] of ref Frame;
    for (index := 0; index < count; index++) {
        (status, seq, pc, extent, names, paths, bits) := ctl->residentframe(id, ticket, index);
        if (status != ctl->DiagReady) {
            return (status, nil);
        }
        if (seq != sequence) {
            return (ctl->DiagInvalid, nil);
        }
        (ns, name) := label(ctl, id, ticket, index, 0, names);
        if (ns != ctl->DiagReady) {
            return (ns, nil);
        }
        (ps, path) := label(ctl, id, ticket, index, 1, paths);
        if (ps != ctl->DiagReady) {
            return (ps, nil);
        }
        frames[index] = ref Frame(pc, extent, bits, name, path);
    }
    return (ctl->DiagReady, ref Stack(sequence, flags, frames));
}


# EN: Own one bounded request through polling/copy/forget; locals allow concurrent
# calls on a shared library. Managed exceptions attempt cleanup before rethrow.
# RU: Владеть bounded request при polling/copy/forget; locals разрешают concurrent
# calls shared library. При managed exceptions пытаться cleanup перед rethrow.
inspect(id: big, attempts: int): (int, ref Stack)
{
    ctl := load Processctl Processctl->PATH;
    sys := load Sys Sys->PATH;
    if (attempts < 1 || attempts > 1000) {
        return (ctl->DiagInvalid, nil);
    }
    (status, ticket) := ctl->residentrequest(id, big 0);
    if (status != ctl->DiagReady) {
        return (status, nil);
    }
    result: ref Stack;
    {
        for (attempt := 0; attempt < attempts; attempt++) {
            (s, sequence, count, flags) := ctl->residentstack(id, ticket);
            status = s;
            if (s != ctl->DiagPending) {
                if (s == ctl->DiagReady) {
                    (status, result) = copy(ctl, id, ticket, sequence, count, flags);
                }
                break;
            }
            sys->sleep(1);
        }
    } exception {
        "*" =>
            ctl->residentforget(id, ticket);
            raise;
    }
    ctl->residentforget(id, ticket);
    return (status, result);
}
