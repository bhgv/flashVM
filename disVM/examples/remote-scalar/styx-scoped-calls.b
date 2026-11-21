# Copyright (c) 2026 bhgv
# EN: Check ordinary load, process selection, and caller-grant lifetime over TCP.
# RU: Проверить load, выбор процесса и срок выдачи контекста при обмене по TCP.
implement Client;
include "sys.m";
include "scoped-calls.m";

Client: module { init: fn(); };


# EN: Invoke with this child's PID and report its result to the parent.
# RU: Вызвать с PID текущего дочернего процесса и вернуть результат родителю.
worker(sys: Sys, service: Scopedcalls, done: chan of (int,big))
{
    pid := sys->pctl(0, nil);
    (seen, code) := service->inspect(0);
    if (seen != pid) {
        raise "provider saw another caller";
    }
    done <-= (pid, code);
}


# EN: Check the current process, two spawned callers, faults and baseline namespace.
# RU: Проверить текущий процесс, два дочерних процесса, отказ и исходный namespace.
init()
{
    sys := load Sys Sys->PATH;
    service := load Scopedcalls "/peer/mounted/scoped-calls.dis";
    if (sys->open("expect-unbound-refusal", Sys->OREAD) != nil) {
        if (service != nil) {
            raise "scoped load accepted without reverse binding";
        }
        report(sys, "PASS scoped compiled load requires reverse mount");
        return;
    }
    if (service == nil) {
        raise "scoped compiled load failed: " + sys->sprint("%r");
    }
    pid := sys->pctl(0, nil);
    (seen, code) := service->inspect(0);
    if (service->baseline(18) != 18 || seen != pid || code != big 1) {
        raise "parent's scoped call failed";
    }
    {
        service->inspect(1);
        raise "expected scoped exception";
    } exception {
        "remote scalar owner error" =>
            ;
    }
    done := chan of (int,big);
    spawn worker(sys, service, done);
    spawn worker(sys, service, done);
    (first, a) := <-done;
    (second, b) := <-done;
    if (first <= 0 || second <= 0 || first == second || first == pid || second == pid ||
        !((a == big 2 && b == big 3) || (a == big 3 && b == big 2)) || service->baseline(19) != 19) {
        raise "scoped child calls lost PID or shared fd cursor";
    }
    checkrefs(service, pid);
    report(sys, "PASS compiled call sees scoped caller VFS, refs and Sysevents");
}


# EN: Check ref copy-back, aliasing, owner failure and event-inbox retirement.
# RU: Проверить обратную запись ref, алиас, отказ владельца и закрытие очереди.
checkrefs(service: Scopedcalls, pid: int)
{
    a := ref Cell(21, big 0);
    if (service->update(a) != pid || a.narrow != pid) {
        raise "single ref did not use caller scope";
    }
    service->updatepair((a, a));
    if (a.narrow != pid+2 || service->baseline(30) != 30) {
        raise "aliased refs or private event retirement failed";
    }
    b := ref Cell(8, big 0);
    service->updatepair((a, b));
    if (a.narrow != pid || b.narrow != 10 || service->baseline(31) != 31) {
        raise "distinct refs or private event retirement failed";
    }
    a.narrow = -1;
    {
        service->updatepair((a, b));
        raise "expected scoped ref failure";
    } exception {
        "remote scalar owner error" =>
            ;
    }
    if (a.narrow != -1 || b.narrow != 10 || service->baseline(32) != 32) {
        raise "failed scoped ref applied copy-back or retained event scope";
    }
}


# EN: Record test completion in the caller VFS, where stdio is not granted.
# RU: Отметить завершение в VFS клиента, где stdio не выдан.
report(sys: Sys, message: string)
{
    output := sys->open("scope-test-output", Sys->OWRITE);
    if (output == nil || sys->fprint(output, "%s\n", message) <= 0) {
        raise "could not report scoped call test";
    }
}
