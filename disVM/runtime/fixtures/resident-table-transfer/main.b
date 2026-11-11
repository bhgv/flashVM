# Copyright (c) 2026 bhgv
# EN: Two-table FIFO transfer, cancellation, retry and aliasing on permanent residents.
# RU: Переход между FIFO двух таблиц, отмена, повтор и алиасы в постоянных потоках.
implement TableTransfer;
include "sys.m";
include "bigtable.m";
include "processctl.m";

TableTransfer: module { init: fn(); };
sys: Sys; # Immutable builtin interface published before spawn.
tables: Bigtable; # Independently guarded native table owners.
control: Processctl; # Exact parent-scoped cancellation and retirement replies.
Rounds: con 40; # Eight repetitions of five distinct queue boundaries.


# EN: Wait for observed FIFO occupancy, borrowing tab; sleep only yields the OS.
# RU: Ждать наблюдаемой длины FIFO, заимствуя tab; sleep уступает только ОС.
queued(tab: ref Bigtable->Handle, expected: int)
{
    for (attempt := 0; attempt < 10000; attempt++) {
        (nil, nil, count) := tables->guardstate(tab);
        if (count == expected) {
            return;
        }
        sys->sleep(1);
    }
    raise "two-table FIFO occupancy did not converge";
}


# EN: Require no lease or waiter on borrowed tab without acquiring its payload.
# RU: Требовать отсутствия lease и ожидающих на tab без захвата payload.
clear(tab: ref Bigtable->Handle)
{
    (owner, depth, waiting) := tables->guardstate(tab);
    if (owner != big 0 || depth || waiting) {
        raise "two-table retry retained a partial reservation";
    }
}


# EN: Hold exactly one table until gate; all arguments remain rooted in this frame.
# RU: Держать ровно одну таблицу до gate; все аргументы укоренены этим кадром.
holder(tab: ref Bigtable->Handle, ready, gate, done: chan of int)
{
    tables->acquire(tab);
    ready <-= control->pid();
    <-gate;
    tables->release(tab);
    done <-= control->pid();
}


# EN: Retry only caught preflight cancellation; report original PID on each reply.
# dest/source are rooted throughout both FIFO waits; no explicit lease is retained.
# RU: Повторять только после отмены preflight; каждый ответ содержит исходный PID.
# dest/source укоренены при обоих ожиданиях FIFO; явного lease нет.
splice(dest, source: ref Bigtable->Handle, ids, cancelled, retry, done: chan of int)
{
    ids <-= control->pid();
    for (;;) {
        {
            tables->requireok(tables->splice(dest, big 0, big 1, source));
            done <-= control->pid();
            return;
        } exception {
            "table lease wait cancelled" => {}
        }
        cancelled <-= control->pid();
        <-retry;
    }
}


# EN: Retire a borrowed direct child after its final acknowledgement; natural exit
# may have completed first, while any other reply loses exact retirement semantics.
# RU: Дождаться прямого ребёнка после последнего подтверждения; естественный выход
# мог уже завершиться, любой иной ответ нарушает точность завершения.
retired(pid: int)
{
    result := control->killwait(pid);
    if (result != Processctl->Retired && result != Processctl->Denied) {
        raise "two-table child did not retire";
    }
}


# EN: Exercise kill at either FIFO, cancel before/after transfer, and alias retry.
# Every round creates three OS residents and checks exact payloads and empty guards.
# RU: Проверить kill в обоих FIFO, отмену до/после перехода и повтор с алиасом.
# Каждый раунд создаёт три потока ОС и проверяет точные данные и пустые guard.
init()
{
    sys = load Sys Sys->PATH;
    tables = load Bigtable Bigtable->PATH;
    control = load Processctl Processctl->PATH;
    for (round := 0; round < Rounds; round++) {
        mode := round % 5;
        left := tables->create();
        right := tables->create();
        tables->requireok(tables->setindex(left, big 0, big 111));
        tables->requireok(tables->setindex(right, big 0, big 222));
        ids := chan of int;
        gateleft := chan of int;
        gateright := chan of int;
        doneleft := chan of int;
        doneright := chan of int;
        done := chan[1] of int;
        cancelled := chan of int;
        retry := chan of int;
        spawn holder(left, ids, gateleft, doneleft);
        first := <-ids;
        spawn holder(right, ids, gateright, doneright);
        second := <-ids;
        if (mode == 4) {
            spawn splice(left, left, ids, cancelled, retry, done);
        } else {
            spawn splice(left, right, ids, cancelled, retry, done);
        }
        target := <-ids;
        queued(left, 1);
        if (mode == 2 || mode == 4) {
            if (control->cancelwait(target) != Processctl->Cancelled || <-cancelled != target) {
                raise "first-table cancellation changed resident identity";
            }
            queued(left, 0);
            retry <-= 1;
            queued(left, 1);
        }
        if (mode == 0) {
            if (control->killwait(target) != Processctl->Retired) {
                raise "first-table queued splice did not retire";
            }
            queued(left, 0);
        }
        gateleft <-= 1;
        if (<-doneleft != first) {
            raise "first table owner identity changed";
        }
        retired(first);
        if (mode != 0 && mode != 4) {
            queued(right, 1);
            clear(left);
            (status, value) := tables->getindex(left, big 0);
            if (status != Bigtable->Ok || value != big 111) {
                raise "splice mutated or retained its first table while waiting for second";
            }
            if (mode == 3) {
                if (control->cancelwait(target) != Processctl->Cancelled || <-cancelled != target) {
                    raise "second-table cancellation changed resident identity";
                }
                queued(right, 0);
                clear(left);
                retry <-= 1;
                queued(right, 1);
                clear(left);
            } else {
                if (control->killwait(target) != Processctl->Retired) {
                    raise "second-table queued splice did not retire";
                }
                queued(right, 0);
            }
        }
        gateright <-= 1;
        if (<-doneright != second) {
            raise "second table owner identity changed";
        }
        retired(second);
        if (mode == 3 || mode == 4) {
            if (<-done != target) {
                raise "retried splice completed under a different resident";
            }
            retired(target);
        }
        clear(left);
        clear(right);
        (status, value) := tables->getindex(left, big 0);
        expected := big 111;
        if (mode == 3) {
            expected = big 222;
        }
        (otherstatus, other) := tables->getindex(right, big 0);
        if (status != Bigtable->Ok || value != expected || otherstatus != Bigtable->Ok ||
                other != big 222 || tables->count(left) != big 1 || tables->count(right) != big 1) {
            raise "cancelled or aliased splice changed exact table payloads";
        }
        alt {
            <-done => raise "duplicate or forbidden splice completion";
            * => {}
        }
    }
    sys->print("PASS two-table queue transfer rounds=40 owners=120 modes=5\n");
}
