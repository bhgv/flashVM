# Copyright (c) 2026 bhgv
# EN: Abandon child A's retirement wait, then wait for child B with mailbox debt.
# RU: Отозвать ожидание завершения A и ждать B при старых ответах в mailbox.
implement RetirementStorage;
include "sys.m";
include "processctl.m";
RetirementStorage: module { init: fn(); };
sys: Sys; # Immutable interface published before spawning residents.
control: Processctl; # Requests always use the permanent caller identity.


# EN: Publish identity, then wait on gate until the parent's real kill request.
# ids/gate are shared roots; no normal channel publication is expected.
# RU: Сообщить PID и ждать gate до настоящего kill родителя.
# ids/gate — общие корни; штатная передача в канал не ожидается.
child(ids, gate: chan of int)
{
    ids <-= control->pid();
    <-gate;
    raise "retirement child unexpectedly resumed its channel";
}


# EN: Wait for A, catch cancellation of that wait and require exact retirement B.
# Shared channels outlive this frame. Host observes genuine prepared calls.
# RU: Ждать A, поймать отмену этого ожидания и потребовать точное завершение B.
# Общие каналы живут дольше кадра. Хост наблюдает настоящие подготовленные вызовы.
parent(ids, done: chan of int)
{
    ids <-= control->pid();
    childids := chan of int;
    gate := chan of int;
    spawn child(childids, gate);
    first := <-childids;
    spawn child(childids, gate);
    second := <-childids;
    retry := 1;
    while (retry) {
        retry = 0;
        {
            control->killwait(first);
            raise "retirement A returned before wait cancellation";
        } exception {
            "out of memory: main" => retry = 1;
            "child exit wait cancelled" => {}
        }
    }
    if (control->killwait(second) != Processctl->Retired) {
        raise "retirement B did not complete exactly";
    }
    done <-= 1;
}


# EN: Wait for the observer's child-A checkpoint, cancel its parent's wait,
# then require both descendants to retire through their own native owners.
# RU: Дождаться контрольной точки A, отменить ожидание родителя и потребовать
# завершения обоих потомков на их собственных native-потоках.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    ids := chan of int;
    done := chan of int;
    spawn parent(ids, done);
    target := <-ids;
    marker := chan[101] of int;
    if (marker == nil || control->cancelwait(target) != Processctl->Cancelled || <-done != 1) {
        raise "retirement storage protocol did not complete";
    }
    sys->print("PASS retirement A abandoned and B acknowledged\n");
}
