# Copyright (c) 2026 bhgv
# EN: Parallel real bytecode/native arithmetic with independent accepted-call accounting.
# RU: Параллельная настоящая арифметика с независимым учётом принятых native-вызовов.
implement NativeCount;
include "sys.m";
include "processctl.m";

NativeCount: module { init: fn(); };
sys: Sys; # Immutable builtin interfaces initialized before spawn.
control: Processctl; # Parent waits for exact child retirement.
Workers: con 8; # Concurrent permanently assigned execution threads.
Rounds: con 2048; # Private arithmetic steps per child, without VM quanta.


# EN: Keep arithmetic state private, then send value/PID through a real channel.
# id selects initial state; ready/start/done are borrowed shared endpoints.
# RU: Хранить арифметическое состояние лично и вернуть value/PID настоящим каналом.
# id задаёт начальное значение; ready/start/done — общие заимствованные каналы.
worker(id: int, ready, start: chan of int, done: chan of (int, int))
{
    value := id;
    ready <-= 1;
    <-start;
    for (at := 0; at < Rounds; at++) {
        value += at;
        value ^= 16r5a5a;
        value ^= 16r5a5a;
    }
    done <-= (value, control->pid());
}


# EN: Start all residents, verify exact values and retire before stats are sampled.
# RU: Запустить все потоки, проверить значения и завершить до снимка статистики.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    ready := chan of int;
    start := chan of int;
    done := chan of (int, int);
    for (id := 0; id < Workers; id++) {
        spawn worker(id, ready, start, done);
    }
    for (id = 0; id < Workers; id++) {
        <-ready;
    }
    for (id = 0; id < Workers; id++) {
        start <-= 1;
    }
    seen := array[Workers] of {* => 0};
    for (id = 0; id < Workers; id++) {
        (value, pid) := <-done;
        owner := value - Rounds * (Rounds - 1) / 2;
        if (owner < 0 || owner >= Workers || seen[owner]) {
            raise "native counter fixture changed real arithmetic or completion identity";
        }
        seen[owner] = 1;
        result := control->killwait(pid);
        if (result != Processctl->Retired && result != Processctl->Denied) {
            raise "native counter fixture did not retire its real producer";
        }
    }
    sys->print("PASS resident native counters workers=8 rounds=2048\n");
}
