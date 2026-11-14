# Copyright (c) 2026 bhgv
# EN: Guest-visible live resident cards under real channel waits and shared scalar publication.
# RU: Guest-visible live resident cards при actual channel waits и shared scalar publication.
implement LiveResidents;
include "sys.m";
include "processctl.m";
LiveResidents: module { init: fn(); };
control: Processctl; # Immutable guest diagnostic/control capability.


# EN/RU: Shared scalar output with neighboring canaries; no pointer or managed graph payload.
# Shared scalar output с neighboring canaries; без pointer либо managed graph payload.
Result: adt
{
    before: int; # Untouched leading canary.
    status: int; # One copied observation status.
    id: big; # Copied lifetime cursor.
    pid: int; # Copied permanent guest PID.
    done: int; # Copied native completion flag.
    after: int; # Untouched trailing canary.
};


# EN/RU: Publish own PID, wait on original channel, then signal completion before native retirement.
# Передать own PID, ждать original channel, затем completion до native retirement.
worker(result: ref Result, ready, release, done: chan of int)
{
    if (result.before != 12345 || result.after != 54321) {
        raise "live resident shared leading canaries";
    }
    sys := load Sys Sys->PATH;
    ready <-= control->pid();
    waiting := 1;
    while (waiting) {
        alt {
            <-release => waiting = 0;
            * => sys->sleep(0);
        }
        if (result.status < -1 || result.status > 1 || result.done < 0 || result.done > 1) {
            raise "live resident shared scalar publication";
        }
    }
    if (result.before != 12345 || result.after != 54321) {
        raise "live resident shared trailing canaries";
    }
    done <-= control->pid();
}


# EN/RU: Enumerate eight held residents through direct shared result fields; verify refusal/joins.
# Перечислить восемь held residents через direct shared result fields; проверить refusal и joins.
init()
{
    sys := load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    ready := chan[8] of int;
    release := chan of int;
    done := chan[8] of int;
    result := ref Result(12345, 0, big 0, 0, 0, 54321);
    for (index := 0; index < 8; index++) {
        spawn worker(result, ready, release, done);
    }
    pids := array[8] of int;
    seen := array[8] of int;
    for (index = 0; index < 8; index++) {
        pids[index] = <-ready;
    }
    cursor := big 0;
    count := 0;
    for (;;) {
        (result.status, result.id, result.pid, result.done) = control->resident(cursor);
        if (result.before != 12345 || result.after != 54321) {
            raise "live resident result overwrote neighboring fields";
        }
        if (result.status == 0) {
            if (result.id != big 0 || result.pid != 0 || result.done != 0) {
                raise "live resident end leaked stale fields";
            }
            break;
        }
        if (result.status != 1 || result.id <= cursor || result.pid <= 0 || result.done != 0 || count >= 8) {
            raise "live resident guest card malformed";
        }
        found := 0;
        for (index = 0; index < 8; index++) {
            if (pids[index] == result.pid) {
                if (seen[index]) {
                    raise "live resident PID duplicated";
                }
                seen[index] = 1;
                found = 1;
            }
        }
        if (!found) {
            raise "live resident card did not identify actual child";
        }
        cursor = result.id;
        count++;
    }
    if (count != 8) {
        raise "live resident enumeration lost a held child";
    }
    (status, id, pid, retired) := control->resident(big -1);
    if (status != -1 || id != big 0 || pid != 0 || retired != 0) {
        raise "live resident invalid cursor published data";
    }
    for (index = 0; index < 8; index++) {
        release <-= 1;
    }
    for (index = 0; index < 8; index++) {
        control->killwait(<-done);
    }
    sys->print("PASS guest live residents: exact eight PIDs shared result canaries refusal joined\n");
}
