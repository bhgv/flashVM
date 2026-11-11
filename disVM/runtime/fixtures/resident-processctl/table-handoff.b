# Copyright (c) 2026 bhgv
# EN: Concurrent table queue cancellation, granted-head retirement and lease cleanup.
# RU: Параллельная отмена очереди таблиц, завершение выданной головы и очистка lease.
implement TableHandoff;
include "sys.m";
include "processctl.m";
include "bigtable.m";

TableHandoff: module { init: fn(); };
sys: Sys; # Immutable interface published before spawn.
control: Processctl; # Read-only process control using TLS identity.
tables: Bigtable; # Shared immutable builtin interface, not a shared table handle.
Parents: con 8; # Independent concurrent table families.
Rounds: con 9; # Three repetitions of each cancellation/retirement mode per parent.


# EN: Wait for an exact observed FIFO length while the parent retains its lease.
# tab remains owned by the caller; polling sleeps through ordinary native Sys.
# RU: Дождаться точной длины FIFO, пока родитель держит lease.
# tab принадлежит вызывающему; опрос уступает ядро через обычный native Sys.
waitqueued(tab: ref Bigtable->Handle, count: int)
{
    for (attempt := 0; attempt < 10000; attempt++) {
        (nil, nil, waiting) := tables->guardstate(tab);
        if (waiting == count) {
            return;
        }
        sys->sleep(1);
    }
    raise "table contenders did not reach the expected FIFO state";
}


# EN: Announce the PID, acquire the table, then await a gate that never opens.
# mode two catches cancellation and acknowledges before natural retirement.
# RU: Сообщить PID, захватить таблицу и ждать gate, который не открывается.
# Режим два подтверждает отмену перед естественным завершением.
victim(tab: ref Bigtable->Handle, ids, acquired, gate, cancelled: chan of int, mode: int)
{
    ids <-= control->pid();
    {
        tables->acquire(tab);
        acquired <-= 1;
        <-gate;
        tables->release(tab);
        raise "table victim continued past its closed gate";
    } exception {
        "table lease wait cancelled" => {
            if (mode != 2) {
                raise "table victim received unexpected cancellation";
            }
            cancelled <-= 1;
        }
    }
}


# EN: Commit exactly one value after the earlier victim leaves the FIFO/lease.
# tab/done are shared; value is copied into native table storage.
# RU: Записать одно значение после ухода жертвы из FIFO/lease.
# tab/done общие; value копируется в нативную память таблицы.
survivor(tab: ref Bigtable->Handle, value: big, done: chan of int)
{
    tables->acquire(tab);
    tables->requireok(tables->setindex(tab, big 0, value));
    tables->release(tab);
    done <-= 1;
}


# EN: Run all three cancellation boundaries with a stable parent Prog/OS owner.
# done acknowledges only after every table value, queue and lease is checked.
# RU: Проверить три границы отмены с неизменным Prog/OS-владельцем родителя.
# done подтверждает завершение после проверки всех значений, очередей и lease.
parent(done: chan of int)
{
    for (round := 0; round < Rounds; round++) {
        tab := tables->create();
        ids := chan of int;
        acquired := chan[1] of int;
        gate := chan of int;
        cancelled := chan[1] of int;
        survivor_done := chan of int;
        mode := round % 3;
        value := big (1000 + round);
        tables->acquire(tab);
        spawn victim(tab, ids, acquired, gate, cancelled, mode);
        target := <-ids;
        waitqueued(tab, 1);
        spawn survivor(tab, value, survivor_done);
        waitqueued(tab, 2);
        if (mode == 1) {
            tables->release(tab);
            if (round == 4 && <-acquired != 1) {
                raise "victim did not acquire its issued table lease";
            }
        }
        if (mode == 2) {
            if (control->cancelwait(target) != Processctl->Cancelled || <-cancelled != 1) {
                raise "table queue head cancellation failed";
            }
            retired := control->killwait(target);
            if (retired != Processctl->Retired && retired != Processctl->Denied) {
                raise "cancelled table child did not retire";
            }
        } else {
            if (control->killwait(target) != Processctl->Retired) {
                raise "table queue head retirement failed";
            }
        }
        if (mode != 1) {
            (token, depth, waiting) := tables->guardstate(tab);
            if (token == big 0 || depth != 1 || waiting != 1) {
                raise "table cancellation changed parent lease or surviving waiter";
            }
            tables->release(tab);
        }
        if (<-survivor_done != 1) {
            raise "table successor was not admitted";
        }
        (status, stored) := tables->getindex(tab, big 0);
        (token, depth, waiting) := tables->guardstate(tab);
        if (status != Bigtable->Ok || stored != value || token != big 0 || depth || waiting) {
            raise "table handoff left a lease, waiter or incorrect value";
        }
        keys := tables->keysof(tab);
        if (tables->keylen(keys) != big 1 || tables->keybig(keys, big 0) != big 0) {
            raise "table key snapshot changed after cancellation";
        }
        if (control->kill(target) != Processctl->Denied || control->cancelwait(target) != Processctl->Denied) {
            raise "retired table child accepted another request";
        }
    }
    done <-= 1;
}


# EN: Keep eight table families active and require all 72 handoffs to finish.
# RU: Запустить восемь семей таблиц и проверить завершение всех 72 передач.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    tables = load Bigtable Bigtable->PATH;
    done := chan of int;
    for (id := 0; id < Parents; id++) {
        spawn parent(done);
    }
    for (completed := 0; completed < Parents; completed++) {
        if (<-done != 1) {
            raise "table handoff parent did not finish";
        }
    }
    sys->print("PASS resident table handoff tables=%d parents=%d\n", Parents * Rounds, Parents);
}
