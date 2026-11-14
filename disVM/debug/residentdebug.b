# Copyright (c) 2026 bhgv
# EN: Assemble immutable native ticket labels into caller-owned UTF-8 strings.
# RU: Собрать неизменяемые native ticket labels в UTF-8 строки своего caller.
implement Residentdebug;
include "sys.m";
include "processctl.m";
include "residentdebug.m";


# EN: Borrow ctl/id/ticket; copy length bytes (part 0=name, 1=path). Missing target
# discards partial bytes. Allocation belongs to caller; no target lifetime pin.
# RU: Заимствовать ctl/id/ticket; копировать length bytes (part 0=name, 1=path).
# При исчезновении target отбросить partial bytes. Allocation своя, без target pin.
label(ctl: Processctl, id, ticket: big, part, length: int): (int, string)
{
    bytes := array[length] of byte;
    for (index := 0; index < length; index++) {
        (status, value) := ctl->residentlocationbyte(id, ticket, part, index);
        if (status != ctl->DiagReady) {
            return (status, nil);
        }
        bytes[index] = byte value;
    }
    return (ctl->DiagReady, string bytes);
}


# EN: Own one request from submit through forget, including allocation/error paths.
# attempts bounds polling, not total latency of native work or scheduler admission.
# RU: Владеть request от submit до forget, включая allocation/error paths.
# attempts ограничивает polling, не latency native work либо scheduler admission.
inspect(id: big, attempts: int): (int, ref Location)
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
    result: ref Location;
    {
        for (attempt := 0; attempt < attempts; attempt++) {
            (s, sequence, pc, count, names, paths, flags) := ctl->residentlocation(id, ticket);
            status = s;
            if (s != ctl->DiagPending) {
                if (s == ctl->DiagReady) {
                    (ns, name) := label(ctl, id, ticket, 0, names);
                    status = ns;
                    if (ns == ctl->DiagReady) {
                        (ps, path) := label(ctl, id, ticket, 1, paths);
                        status = ps;
                        if (ps == ctl->DiagReady) {
                            result = ref Location(sequence, pc, count, flags, name, path);
                        }
                    }
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
