# Copyright (c) 2026 bhgv
# EN: Existing Unibus calls resolve on the correct real guest before any application setup.
# RU: Существующие вызовы Unibus принадлежат своему guest до настройки приложения.
implement Check;
include "sys.m";
include "unibus.m";
include "processctl.m";
Check: module { init: fn(); };


# EN/RU: Fail an actual interpreter/JIT contract explicitly. / Явно проверить контракт VM.
check(ok: int, why: string)
{
    if (!ok) {
        raise why;
    }
}


# EN/RU: A permanent child receives only explicitly inherited native authority.
# Постоянный child owner получает только явно наследуемые native-права.
child(host: Unibus, inherited: int, expected: string, ready: chan of int)
{
    ctl := load Processctl Processctl->PATH;
    if (inherited) {
        check(host->rows() == 47 && host->path() == expected, "inherited handler data");
        input := chan of array of byte;
        output := chan of array of byte;
        check(host->attach(input, output) == -1, "child stole root attachment");
        denied := 0;
        {
            host->detach();
        } exception {
        "Unibus operation belongs to the launch owner" =>
            denied = 1;
        }
        check(denied, "child detached root ports");
    } else {
        denied := 0;
        {
            host->rows();
        } exception {
        "Unibus is not bound to this process" =>
            denied = 1;
        }
        check(denied, "non-inherited child accessed a host fallback");
    }
    ready <-= ctl->pid();
    never := chan of int;
    <-never;
}


# EN/RU: Use the unchanged .m interface and verify child rejection cannot detach the root.
# Использовать прежний .m, проверить, что отказ ребёнку не отключает root.
init()
{
    sys := load Sys Sys->PATH;
    host := load Unibus Unibus->PATH;
    ctl := load Processctl Processctl->PATH;
    check(host != nil && host->rows() == 47 && host->linenumbers() == 1,
        "Unibus was not bound before init");
    expected := host->path();
    check(expected == "job-native" || expected == "job-native-revoke", "private native launch data");
    unavailable := 0;
    {
        host->ink(1);
    } exception {
    "Unibus operation unavailable for this launch" =>
        unavailable = 1;
    }
    check(unavailable, "missing native operation incorrectly used local fallback");
    inherited := host->fresh();
    input := chan of array of byte;
    output := chan of array of byte;
    check(host->attach(input, output) == 0, "root registration");
    ready := chan of int;
    spawn child(host, inherited, expected, ready);
    pid := <-ready;
    check(host->rows() == 47 && ctl->killwait(pid) == Processctl->Retired, "root survives child calls");
    host->detach();
    if (expected == "job-native-revoke") {
        revoked := 0;
        {
            host->rows();
        } exception {
        "retired Unibus binding" =>
            revoked = 1;
        }
        check(revoked, "revoked handler was still callable");
    }
    sys->print("PASS real pre-init Unibus dispatch and isolation\n");
}
