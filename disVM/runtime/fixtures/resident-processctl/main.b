# Copyright (c) 2026 bhgv
# EN: Concurrent parents cancel and retire their own children through mailboxes.
# RU: Параллельные родители отменяют ожидания и завершают своих детей сообщениями.
implement ResidentProcessctl;
include "sys.m";
include "processctl.m";

ResidentProcessctl: module { init: fn(); };
sys: Sys; # Immutable Sys interface, initialized before spawn.
control: Processctl; # Immutable process-control interface.
Parents: con 8; # Concurrent independent parent threads.
Rounds: con 16; # New child identities tested per parent.


# EN: A private mapped object kept live across cancellation and final termination.
# RU: Приватный mapped-объект, живой при отмене и окончательном завершении.
Cell: adt {
    value: int; # Set by this child before publication of its PID.
};


# EN: Publish this child's PID, wait for cancellation, verify roots and acknowledge.
# All channels are borrowed shared owners; the private graph lives until killwait.
# RU: Опубликовать PID, ждать отмены, проверить корни и подтвердить отмену.
# Каналы — общие ссылки; приватный граф живёт до killwait.
child(ids, reports, gate: chan of int, native: int)
{
    roots := array[128] of ref Cell;
    for (index := 0; index < len roots; index++) {
        roots[index] = ref Cell(index + 1000);
    }
    ids <-= control->pid();
    {
        choices := array[3] of chan of int;
        choices[1] = gate;
        choices[2] = gate;
        selected: (int, int);
        alt {
            selected = <-choices => raise "unexpected child gate publication";
        }
        raise "cancelled ALT returned normally";
    } exception {
        "channel wait cancelled" => {}
    }
    for (verify := 0; verify < len roots; verify++) {
        if (roots[verify] == nil || roots[verify].value != verify + 1000) {
            raise "private child roots changed during cancellation";
        }
    }
    reports <-= 1;
    if (native) {
        sys->sleep(60000);
    } else {
        <-gate;
    }
    raise "child continued after retirement request";
}


# EN: Exercise only direct children; every correlated reply must be consumed once.
# id selects this parent's ring slot; done signals completion, foreign/checked
# synchronize live foreign-child denial. native selects the final wait kind.
# RU: Управлять только своими детьми; каждый ответ принять ровно один раз.
# id задаёт место родителя в кольце; done сообщает о завершении, foreign/checked
# согласуют отказ для чужого живого ребёнка. native задаёт последнее ожидание.
parent(id: int, done: chan of int, foreign, checked: array of chan of int, native: int)
{
    for (round := 0; round < Rounds; round++) {
        ids := chan of int;
        reports := chan of int;
        gate := chan of int;
        spawn child(ids, reports, gate, native);
        target := <-ids;
        foreign[(id + 1) % Parents] <-= target;
        other := <-foreign[id];
        if (control->kill(other) != Processctl->Denied ||
            control->killwait(other) != Processctl->Denied ||
            control->cancelwait(other) != Processctl->Denied) {
            raise "foreign parent was allowed to control a live child";
        }
        checked[(id + Parents - 1) % Parents] <-= 1;
        if (<-checked[id] != 1) {
            raise "foreign authorization check was not acknowledged";
        }
        attempts := 0;
        for (;;) {
            result := control->cancelwait(target);
            if (result == Processctl->Cancelled) {
                break;
            }
            if (result != Processctl->NotWaiting || attempts++ > 10000) {
                raise "cancelwait lost child or correlated result";
            }
            sys->sleep(1);
        }
        if (<-reports != 1 || control->killwait(target) != Processctl->Retired) {
            raise "killwait lost cancellation acknowledgement or retirement";
        }
        if (control->killwait(target) != Processctl->Denied ||
            control->cancelwait(target) != Processctl->Denied ||
            control->kill(target) != Processctl->Denied) {
            raise "retired child accepted a duplicate operation";
        }
        alt {
            <-reports => raise "duplicate child completion";
            * => {}
        }
    }
    done <-= 1;
}


# EN: Keep eight parents resident together; check both final wait kinds.
# No arguments/result; all errors terminate the ordinary VM application.
# RU: Запустить восемь родителей одновременно с обоими видами ожидания.
# Без аргументов/результата; ошибки завершают обычное приложение VM.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    done := chan of int;
    foreign := array[Parents] of chan of int;
    checked := array[Parents] of chan of int;
    for (slot := 0; slot < Parents; slot++) {
        foreign[slot] = chan[1] of int;
        checked[slot] = chan[1] of int;
    }
    for (id := 0; id < Parents; id++) {
        spawn parent(id, done, foreign, checked, id & 1);
    }
    for (completed := 0; completed < Parents; completed++) {
        if (<-done != 1) {
            raise "parent did not complete";
        }
    }
    sys->print("PASS resident process control children=%d parents=%d\n", Parents * Rounds, Parents);
}
