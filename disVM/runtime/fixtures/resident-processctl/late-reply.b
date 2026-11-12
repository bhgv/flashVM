# Copyright (c) 2026 bhgv
# EN: Discard a stale child reply while its live parent crosses native boundaries.
# RU: Отбросить старый ответ ребёнка при native-переходах живого родителя.
implement LateReply;
include "sys.m";
include "processctl.m";

LateReply: module { init: fn(); };
sys: Sys; # Immutable shared interface published before spawning.
control: Processctl; # Immutable interface; caller identity belongs to TLS.


# EN: Publish identity, park in ALT, then preserve roots after cancellation.
# ids/gate/done are borrowed shared channels; root arrays stay owner-local.
# RU: Сообщить PID, ожидать в ALT и сохранить корни после отмены.
# Каналы общие; массивы корней принадлежат этому исполнителю.
child(ids, gate, done: chan of int)
{
    roots := array[64] of array of int;
    for (index := 0; index < len roots; index++) {
        roots[index] = array[1] of int;
        roots[index][0] = index + 4000;
    }
    ids <-= control->pid();
    {
        alt {
            <-gate => raise "unexpected late-reply channel publication";
        }
    } exception {
        "channel wait cancelled" => {}
    }
    for (verify := 0; verify < len roots; verify++) {
        if (roots[verify] == nil || roots[verify][0] != verify + 4000) {
            raise "late reply lost child roots";
        }
    }
    done <-= 1;
}


# EN: Cancel the child; grandparent cancels this wait while its reply is held.
# ids/done are shared. Sentinel sleep is the private native-boundary observer;
# normal return from the first cancelwait before interruption is forbidden.
# RU: Отменить ребёнка; дед отменяет это ожидание при задержанном ответе.
# Каналы общие. Особый sleep — частный наблюдатель native-переходов;
# обычный возврат первого cancelwait до прерывания запрещён.
middle(ids, done: chan of int)
{
    ids <-= control->pid();
    childids := chan of int;
    gate := chan of int;
    spawn child(childids, gate, done);
    target := <-childids;
    roots := array[] of {"parent remains alive", "live registers stay local"};
    {
        for (;;) {
            if (control->cancelwait(target) != Processctl->NotWaiting) {
                raise "stale reply resumed the old parent call";
            }
            sys->sleep(1);
        }
    } exception {
        "child exit wait cancelled" => {}
    }
    if (sys->sleep(42423) != 0 || roots[0] != "parent remains alive" ||
        roots[1] != "live registers stay local") {
        raise "late reply changed the parent's new continuation";
    }
    done <-= 2;
}


# EN: Wait for the held child reply, then cancel only its parent's prepared call.
# Own both shared completion channels until both guests finish; errors propagate.
# RU: Дождаться задержанного ответа и отменить лишь подготовленный вызов родителя.
# Удерживать каналы до завершения обоих гостей; ошибки передаются далее.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    ids := chan of int;
    done := chan of int;
    spawn middle(ids, done);
    target := <-ids;
    entered := chan[101] of int;
    if (entered == nil || control->cancelwait(target) != Processctl->Cancelled) {
        raise "parent prepared call was not cancelled";
    }
    first := <-done;
    second := <-done;
    if (first + second != 3 || first == second) {
        raise "late reply duplicated or lost completion";
    }
    sys->print("PASS stale reply discarded for live native parent\n");
}
