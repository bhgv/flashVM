# Copyright (c) 2026 bhgv
# EN: Check ordinary load, process selection, and caller-grant lifetime over TCP.
# RU: Проверить load, выбор процесса и срок выдачи контекста при обмене по TCP.
implement Client;
include "sys.m";
include "processctl.m";
include "scoped-calls.m";
include "vfsexport.m";

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
    if (sys->open("expect-unbound-refusal", Sys->OREAD) != nil) {
        service := load Scopedcalls "/peer/mounted/scoped-calls.dis";
        if (service != nil) {
            raise "scoped load accepted without reverse binding";
        }
        report(sys, "PASS scoped compiled load requires reverse mount");
        return;
    }
    exports := load Vfsexport Vfsexport->PATH;
    cell := exports->intvar(41);
    rights := Vfsexport->Read | Vfsexport->Execute;
    base := exports->capture("/.var", 0, rights);
    vars := exports->overlay(base, cell, "/live", Vfsexport->Read, 0);
    first_cell := exports->intvar(7);
    second_cell := exports->intvar(9);
    if (exports->intarray(base :: nil) != nil) {
        raise "integer collection admitted a directory";
    }
    collection := exports->intarray(first_cell :: second_cell :: first_cell :: nil);
    vars = exports->overlay(vars, collection, "/vector", rights | Vfsexport->Write, Vfsexport->Tree);
    nested := exports->record(("vector", collection) :: nil);
    record := exports->record(("value", first_cell) :: ("nested", nested) :: nil);
    if (nested == nil || record == nil ||
        exports->record(("dup", first_cell) :: ("dup", second_cell) :: nil) != nil ||
        exports->record((".len", first_cell) :: nil) != nil) {
        raise "record accepted invalid fields or did not compose";
    }
    vars = exports->overlay(vars, record, "/record", rights | Vfsexport->Write, Vfsexport->Tree);
    if (base == nil || cell == nil || vars == nil || exports->mount(vars, "/.var", 0, 0) != 0) {
        raise "could not mount live variable for event test";
    }
    vfs_cell := exports->intvar(42);
    vfs_base := exports->capture("/.vfs", 0, rights);
    vfs := exports->overlay(vfs_base, vfs_cell, "/live", Vfsexport->Read | Vfsexport->Write, 0);
    if (vfs_base == nil || vfs_cell == nil || vfs == nil || exports->mount(vfs, "/.vfs", 0, 0) != 0) {
        raise "could not mount live VFS file for event test";
    }
    private_cell := exports->intvar(17);
    if (private_cell == nil || exports->mount(private_cell, "/private-live", 0, 0) != 0) {
        raise "could not mount unshared variable";
    }
    service := load Scopedcalls "/peer/mounted/scoped-calls.dis";
    if (service == nil) {
        raise "scoped compiled load failed: " + sys->sprint("%r");
    }
    edited := chan of int;
    spawn change_live(sys, service, exports, cell, private_cell, edited);
    pid := sys->pctl(0, nil);
    (seen, code) := service->inspect(0);
    if (<-edited != 1 || service->baseline(18) != 18 || seen != pid || code != big 1) {
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
    other := load Scopedcalls "/peer/mounted/scoped-extra.dis";
    if (other == nil) {
        raise "second scoped module failed to load";
    }
    (caller, executor) := service->inspect(5);
    across := ref Cell(-51, big 0);
    if (caller != pid || executor <= big 0 || other->update(across) != pid || across.wide != executor) {
        raise "different module did not reuse the caller executor";
    }
    checkrefs(service, pid);
    concurrent_refs(sys, service, pid, 0);
    concurrent_refs(sys, service, pid, 1);
    checkcancel(sys, service, other, 3, "compute-cancel-");
    checkcancel(sys, service, other, 2, "cancel-");
    (slowpid, slowvalue) := service->inspect(4);
    if (slowpid != pid || slowvalue != big 44) {
        raise "slow scoped call lost its private reply";
    }
    local_idx := sys->open("/.var/vector/.idx", Sys->OWRITE);
    selected := array[] of {byte 0, byte 0, byte 0, byte 1};
    if (local_idx == nil || sys->write(local_idx, selected, len selected) != len selected) {
        raise "could not select a local array cell";
    }
    (arraypid, nil) := service->inspect(6);
    local_val := sys->open("/.var/vector/.val", Sys->OREAD);
    if (local_val == nil || sys->read(local_val, selected, len selected) != len selected || selected[3] != byte 9) {
        raise "reverse selection replaced the local selection";
    }
    local_idx = nil;
    (valid, updated) := exports->intget(first_cell);
    (valid2, unchanged) := exports->intget(second_cell);
    if (arraypid != pid || !valid || updated != 19 || !valid2 || unchanged != 9) {
        raise "remote array change did not update the local live cells";
    }
    report(sys, "PASS compiled call sees scoped caller VFS, refs and Sysevents");
}


# EN: Check ref copy-back, aliasing, owner failure and event-inbox retirement.
# RU: Проверить обратную запись ref, алиас, отказ владельца и закрытие очереди.
checkrefs(service: Scopedcalls, pid: int)
{
    (caller, executor) := service->inspect(5);
    a := ref Cell(-51, big 0);
    if (caller != pid || executor <= big 0 || service->update(a) != pid || a.wide != executor) {
        raise "dynamic and ref exports used different executors";
    }
    a = ref Cell(-51, big 0);
    service->updatepair((a, a));
    if (a.wide != executor) {
        raise "ref pair export used a different executor";
    }
    a = ref Cell(21, big 0);
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


# EN: Mutate the published variable only after the provider opens its event stream.
# RU: Изменить опубликованную переменную после открытия потока у провайдера.
change_live(sys: Sys, service: Scopedcalls, exports: Vfsexport, cell, private_cell: ref Vfsexport->Root, done: chan of int)
{
    wait_marker(sys, "refpair-entered");
    # EN/RU: The first call is still waiting for this worker. / Первый вызов ещё ждёт этот поток.
    if (service->baseline(57) != 57) {
        raise "another export could not run while the first call was pending";
    }
    success := exports->intset(private_cell, 23) && exports->intset(cell, 65539);
    file := sys->open("/.vfs/live", Sys->OWRITE);
    encoded := array[] of {byte 0, byte 2, byte 0, byte 5};
    if (file == nil || sys->write(file, encoded, len encoded) != len encoded) {
        success = 0;
    }
    set_marker(sys, "refpair-edited");
    wait_marker(sys, "overflow-ready");
    for (i := 1; i <= 33; i++) {
        if (!exports->intset(cell, 65536+i)) {
            success = 0;
        }
    }
    set_marker(sys, "overflow-done");
    wait_marker(sys, "recovery-ready");
    if (!exports->intset(cell, 65536+99)) {
        success = 0;
    }
    set_marker(sys, "recovery-done");
    done <-= success;
}

include "refpair-sync.b";

# EN: Revoke an active call by killing its caller, then reuse the same mount.
# RU: Отозвать активный вызов завершением клиента и повторно использовать маунт.
checkcancel(sys: Sys, service, other: Scopedcalls, mode: int, prefix: string)
{
    ctl := load Processctl Processctl->PATH;
    started := chan of int;
    resumed := chan of int;
    spawn cancel_worker(service, other, ctl, started, resumed, mode);
    child := <-started;
    wait_marker(sys, prefix + "entered");
    (companion, workerpid) := service->inspect(5);
    if (companion != ctl->pid() || workerpid <= big 0) {
        raise "same export could not run for a second caller";
    }
    if (ctl->cancelwait(child) != Processctl->Cancelled) {
        raise "could not cancel the child caller wait";
    }
    wait_marker(sys, prefix + "observed");
    if (<-resumed != 1) {
        raise "same caller did not receive a replacement provider executor";
    }
    (again, replacement) := service->inspect(5);
    if (again != companion || replacement <= big 0 || replacement != workerpid) {
        raise "unaffected caller lost its provider executor";
    }
    {
        service->inspect(1);
        raise "expected owner error after cancellation";
    } exception {
        "remote scalar owner error" =>
            ;
    }
    if (service->baseline(73) != 73) {
        raise "another export failed after cancellation";
    }
}


# EN: Publish the caller PID before entering the remote function.
# RU: Передать PID клиента перед входом в удалённую функцию.
cancel_worker(service, other: Scopedcalls, ctl: Processctl, started, resumed: chan of int, mode: int)
{
    pid := ctl->pid();
    (before_pid, before) := service->inspect(5);
    started <-= pid;
    cancelled := 0;
    {
        service->inspect(mode);
    } exception {
        "*" =>
            cancelled = 1;
    }
    cell := ref Cell(-51, big 0);
    after_pid := other->update(cell);
    (raw_pid, after) := service->inspect(5);
    resumed <-= cancelled && before_pid == pid && after_pid == pid && raw_pid == pid &&
        before > big 0 && after > big 0 && before != after && cell.wide == after;
}

# EN: Run a second ref call on the same export while the first is asleep.
# RU: Выполнить второй вызов ref того же экспорта, пока первый спит.
concurrent_refs(sys: Sys, service: Scopedcalls, pid, pair: int)
{
    ctl := load Processctl Processctl->PATH;
    started := chan of int;
    pending := ref Cell(-50, big 0);
    spawn hold_ref(service, ctl, started, pending, pair);
    child := <-started;
    prefix := "single-ref";
    if (pair) {
        prefix = "pair-ref";
    }
    wait_marker(sys, prefix + "-pending");
    cell := ref Cell(0, big 0);
    if (pair) {
        service->updatepair((cell, cell));
        if (cell.narrow != pid+2) {
            raise "concurrent ref pair lost its alias update";
        }
    } else if (service->update(cell) != pid || cell.narrow != pid) {
        raise "concurrent ref lost its update";
    }
    if (ctl->killwait(child) != Processctl->Retired) {
        raise "ref caller did not retire";
    }
    wait_marker(sys, prefix + "-cancelled");
    if (pending.narrow != -50 || pending.wide != big 0) {
        raise "cancelled ref call changed the caller snapshot";
    }
}


# EN: Publish the caller PID and hold the selected ref export open.
# RU: Сообщить PID клиента и удерживать выбранный экспорт ref открытым.
hold_ref(service: Scopedcalls, ctl: Processctl, started: chan of int, pending: ref Cell, pair: int)
{
    started <-= ctl->pid();
    if (pair) {
        service->updatepair((pending, pending));
    } else {
        service->update(pending);
    }
}
