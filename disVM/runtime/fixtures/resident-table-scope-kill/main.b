# Copyright (c) 2026 bhgv
# EN: Kill a queued primitive, then its admitted owner after dropping the parent root.
# RU: Завершить ожидающую операцию и её владельца после снятия родительской ссылки.
implement TableScopeKill;
include "sys.m";
include "bigtable.m";
include "processctl.m";

TableScopeKill: module { init: fn(); };
sys: Sys; # Immutable builtin interface initialized before spawn.
tables: Bigtable; # Per-table FIFO and implicit primitive guard.
control: Processctl; # Parent-scoped process requests and exact retirement replies.
Rounds: con 32; # Fresh native owner and child pair per round.


# EN: Publish PID, attempt one primitive, then report forbidden post-kill execution.
# tab/key/value/ids/reports remain rooted by this child's permanent frame.
# RU: Сообщить PID, выполнить операцию и сообщить о запрещённом продолжении.
# tab/key/value/ids/reports укоренены постоянным кадром этого ребёнка.
write(tab: ref Bigtable->Handle, key, value: big, ids, reports: chan of int)
{
    ids <-= control->pid();
    tables->requireok(tables->setindex(tab, key, value));
    reports <-= 1;
}


# EN: Require host confirmation of a real native boundary on the borrowed stdin.
# RU: Требовать подтверждение нативной границы хостом через заимствованный stdin.
permit(input: ref Sys->FD, bytes: array of byte)
{
    if (sys->read(input, bytes, 1) != 1 || bytes[0] != byte 49) {
        raise "table scope boundary handshake changed";
    }
}


# EN: Retire the queued peer before releasing the admitted primitive's native call.
# Drop the parent's last table reference, then wait for the admitted child too.
# RU: Завершить ожидающего соседа до выхода допущенной операции из native.
# Снять родительскую ссылку на таблицу и дождаться допущенного ребёнка.
init()
{
    sys = load Sys Sys->PATH;
    tables = load Bigtable Bigtable->PATH;
    control = load Processctl Processctl->PATH;
    input := sys->fildes(0);
    bytes := array[1] of byte;
    for (round := 0; round < Rounds; round++) {
        tab := tables->create();
        ids := chan of int;
        reports := chan[2] of int;
        spawn write(tab, big 0, big 111, ids, reports);
        head := <-ids;
        permit(input, bytes);
        spawn write(tab, big 1, big 222, ids, reports);
        queued := <-ids;
        permit(input, bytes);
        if (control->killwait(queued) != Processctl->Retired) {
            raise "queued table primitive did not retire while the owner was held";
        }
        if (control->cancelwait(head) != Processctl->NotWaiting) {
            raise "admitted table primitive was reported as cancellable";
        }
        tab = nil;
        sys->print("PASS queued table primitive retired phase %d\n", round + 1);
        if (control->killwait(head) != Processctl->Retired) {
            raise "admitted table primitive did not retire after native return";
        }
        if (control->kill(head) != Processctl->Denied || control->kill(queued) != Processctl->Denied ||
                control->cancelwait(head) != Processctl->Denied || control->cancelwait(queued) != Processctl->Denied) {
            raise "dead table primitive accepted another request";
        }
        alt {
            <-reports => raise "table primitive executed after its kill request";
            * => {}
        }
    }
    sys->print("PASS table scope kill rounds=32 owners=64\n");
}
