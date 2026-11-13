# Copyright (c) 2026 bhgv
# EN: A cancelled parent wait followed by another cancelwait on the same child.
# RU: Отозванное ожидание родителя и повторный cancelwait того же ребёнка.
implement CancelGeneration;
include "sys.m";
include "bigtable.m";
include "processctl.m";

CancelGeneration: module { init: fn(); };
sys: Sys; # Immutable interface published before spawn.
tables: Bigtable; # Shared table API, leases remain owner scoped.
control: Processctl; # Caller identity belongs to the permanent TLS owner.


# EN: Park on parent's table, catch genuine cancellation and acknowledge it.
# tab/ids/done are shared frame roots; do not acquire after cancellation.
# RU: Ожидать таблицу родителя, поймать настоящую отмену и подтвердить её.
# tab/ids/done укоренены кадром; после отмены lease не захватывать.
child(tab: ref Bigtable->Handle, ids, done: chan of int)
{
    ids <-= control->pid();
    {
        tables->acquire(tab);
        raise "cancel generation child acquired a locked table";
    } exception {
        "table lease wait cancelled" => {}
    }
    done <-= 1;
}


# EN: Queue A, abandon its prepared call on grandparent cancellation, then post B
# to the same child. A cancels the table wait; B must report NotWaiting.
# Grandparent owns the lease; shared channels preserve completion roots.
# RU: Поставить A, отозвать подготовленный вызов по отмене деда и послать B
# тому же ребёнку. A отменяет таблицу; B обязан вернуть NotWaiting.
# Дед удерживает lease tab; каналы сохраняют корни завершения.
parent(tab: ref Bigtable->Handle, ids, done: chan of int)
{
    ids <-= control->pid();
    childids := chan of int;
    spawn child(tab, childids, done);
    target := <-childids;
    for (;;) {
        (nil, nil, waiting) := tables->guardstate(tab);
        if (waiting == 1) {
            break;
        }
        sys->sleep(1);
    }
    {
        control->cancelwait(target);
        raise "first cancellation returned before abandonment";
    } exception {
        "child exit wait cancelled" => {}
    }
    if (control->cancelwait(target) != Processctl->NotWaiting) {
        raise "stale cancellation A completed new wait B";
    }
    done <-= 2;
}


# EN: Cancel only the parent's pending A; retain completion channels until both
# descendants finish. Host marker makes the critical interleaving deterministic.
# RU: Отменить лишь ожидание A родителя; удерживать каналы до завершения обоих
# потомков. Метка хоста задаёт детерминированное чередование потоков.
init()
{
    sys = load Sys Sys->PATH;
    tables = load Bigtable Bigtable->PATH;
    control = load Processctl Processctl->PATH;
    ids := chan of int;
    done := chan[2] of int;
    tab := tables->create();
    tables->acquire(tab);
    spawn parent(tab, ids, done);
    target := <-ids;
    marker := chan[101] of int;
    if (marker == nil || control->cancelwait(target) != Processctl->Cancelled) {
        raise "grandparent did not abandon cancellation A";
    }
    first := <-done;
    second := <-done;
    if (first + second != 3 || first == second) {
        raise "cancel generation descendants did not finish exactly once";
    }
    tables->release(tab);
    sys->print("PASS cancellation generation rejected stale A and completed B\n");
}
