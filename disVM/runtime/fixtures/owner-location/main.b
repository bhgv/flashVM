# Copyright (c) 2026 bhgv
# EN: Freeze ticket locations across cross-module return, unload and later owner progress.
# RU: Сохранить ticket location после cross-module return, unload и дальнейшей работы owner.
implement OwnerLocation;
include "sys.m";
include "processctl.m";
include "residentdebug.m";
include "provider.m";
OwnerLocation: module { init: fn(); };
ProviderPath: con "provider-λ.dis"; # Driver builds real Unicode module path beside the caller.
ctl: Processctl; # Matched immutable diagnostic/control capability.
sys: Sys; # Matched immutable scalar services.


# EN/RU: Run imported function, drop its last instance, then await direct-child killwait.
# Вызвать imported function, отпустить экземпляр, затем ждать parent killwait.
worker(ids, ready, gate, returned: chan of int)
{
    ids <-= ctl->pid();
    provider := load LocationProvider ProviderPath;
    if (provider == nil || provider->wait(ready, gate) != 130) {
        raise "owner location imported call or payload failed";
    }
    provider = nil;
    returned <-= ctl->pid();
    hold := chan of int;
    <-hold;
}


# EN/RU: Find exact lifetime ID of held PID using copied registry metadata only.
# Найти exact lifetime ID удержанного PID только по copied registry metadata.
identity(pid: int): big
{
    cursor := big 0;
    for (;;) {
        (status, id, found, done) := ctl->resident(cursor);
        if (status != 1 || id <= cursor) {
            raise "owner location registry target missing";
        }
        if (found == pid) {
            return id;
        }
        cursor = id;
    }
}


# EN/RU: Poll immutable ticket header, retaining its credit for later frozen re-read.
# Читать immutable ticket header, сохраняя credit для повторного frozen чтения.
header(id, ticket: big): (int, big, big, int, int, int, int)
{
    for (attempt := 0; attempt < 1000; attempt++) {
        (status, sequence, pc, count, names, paths, flags) := ctl->residentlocation(id, ticket);
        if (status != ctl->DiagPending) {
            return (status, sequence, pc, count, names, paths, flags);
        }
        sys->sleep(1);
    }
    return (ctl->DiagPending, big 0, big 0, 0, 0, 0, 0);
}


# EN/RU: Read fixed bytes from one ticket; loss/denial discards the private partial copy.
# Читать fixed bytes одного ticket; loss/denial отбрасывает private partial copy.
text(id, ticket: big, part, length: int): string
{
    bytes := array[length] of byte;
    for (index := 0; index < length; index++) {
        (status, value) := ctl->residentlocationbyte(id, ticket, part, index);
        if (status != ctl->DiagReady) {
            raise "owner location copied bytes unavailable";
        }
        bytes[index] = byte value;
    }
    return string bytes;
}


# EN/RU: Keep this caller's ticket private even when its number is deliberately shared.
# Сохранить ticket private для caller, даже при явной передаче его номера другому Prog.
thief(id, ticket: big, done: chan of int)
{
    (status, sequence, pc, count, names, paths, flags) := ctl->residentlocation(id, ticket);
    (bs, value) := ctl->residentlocationbyte(id, ticket, 0, 0);
    if (status != ctl->DiagDenied || sequence != big 0 || pc != big 0 || count || names || paths || flags ||
        bs != ctl->DiagDenied || value != 0) {
        raise "owner location foreign ticket projection leaked";
    }
    done <-= ctl->pid();
    hold := chan of int;
    <-hold;
}


# EN: Require original-Dis next-PC and exact module labels. Capture identity is
# ticket/sequence, never a Module handle. Only called while owner remains held.
# RU: Проверить original-Dis next-PC и точные module labels. Identity — ticket/
# sequence, не Module handle. Вызывать только при удержанном owner.
valid(status: int, sequence, pc: big, count, flags: int, name, path: string): int
{
    return status == ctl->DiagReady && sequence > big 0 && pc >= big 0 && pc <= big count &&
           count > 0 && (flags & ctl->LocationValid) && name == "LocationProvider" && path == ProviderPath;
}


# EN: Inspect real imported waits and frozen replies after return/unload. All created
# owners join before positive/precise omission diagnostics; no observer pins target.
# RU: Исследовать imported waits и frozen replies после return/unload. Все owners
# завершаются до positive/точного omission diagnostic; observer не удерживает target.
init()
{
    sys = load Sys Sys->PATH;
    ctl = load Processctl Processctl->PATH;
    debug := load Residentdebug Residentdebug->PATH;
    ids := chan of int;
    ready := chan of int;
    gate := chan of int;
    returned := chan of int;
    done := chan of int;
    spawn worker(ids, ready, gate, returned);
    pid := <-ids;
    if (<-ready != 1) {
        raise "owner location provider entry missing";
    }
    id := identity(pid);
    (status, ticket) := ctl->residentrequest(id, big 41);
    if (status != ctl->DiagReady) {
        raise "owner location request refused";
    }
    (hs, sequence, pc, count, names, paths, flags) := header(id, ticket);
    name := text(id, ticket, 0, names);
    path := text(id, ticket, 1, paths);
    good := valid(hs, sequence, pc, count, flags, name, path);
    spawn thief(id, ticket, done);
    if (ctl->killwait(<-done) != ctl->Retired) {
        raise "owner location foreign caller retirement refused";
    }
    (bad, value) := ctl->residentlocationbyte(id, ticket, 2, 0);
    if (bad != ctl->DiagInvalid || value != 0) {
        raise "owner location invalid part accepted";
    }
    (bad, value) = ctl->residentlocationbyte(id, ticket, 0, names);
    if (bad != ctl->DiagInvalid || value != 0) {
        raise "owner location out-of-bounds byte accepted";
    }
    gate <-= 123;
    if (<-returned != pid) {
        raise "owner location return changed permanent PID";
    }
    (frozen, s2, p2, n2, a2, b2, f2) := header(id, ticket);
    if (frozen != hs || s2 != sequence || p2 != pc || n2 != count || a2 != names || b2 != paths || f2 != flags ||
        text(id, ticket, 0, names) != name || text(id, ticket, 1, paths) != path) {
        raise "owner location ticket mutated after module return";
    }
    (fresh, location) := debug->inspect(id, 1000);
    if (good && (fresh != ctl->DiagReady || location == nil || !(location.flags & ctl->LocationValid) ||
        location.sequence <= sequence || location.name != "OwnerLocation")) {
        raise "owner location subsequent caller boundary missing";
    }
    (reply, owner, cookie, d, t, w) := ctl->residentreply(id, ticket);
    if (reply != ctl->DiagReady || owner != pid || cookie != big 41) {
        raise "owner location peek consumed ordinary reply";
    }
    (missing, ms, mp, mn, ma, mb, mf) := ctl->residentlocation(id, ticket);
    if (missing != ctl->DiagMissing || ms != big 0 || mp != big 0 || mn || ma || mb || mf) {
        raise "owner location consumed ticket aliased snapshot";
    }
    if (ctl->killwait(pid) != ctl->Retired) {
        raise "owner location target retirement refused";
    }
    if (!good) {
        sys->print("DETECTED owner location missing or wrong copied module labels\n");
        return;
    }
    sys->print("PASS owner location: imported Unicode labels next PC frozen ticket later boundary library ownership joined\n");
}
