# Copyright (c) 2026 bhgv
# EN: Parallel real mapped slices with independent completion accounting.
# RU: Параллельные настоящие mapped-срезы с независимым учётом завершений.
implement MappedCount;
include "sys.m";
include "processctl.m";

MappedCount: module { init: fn(); };
sys: Sys; # Immutable builtin interfaces initialized before spawn.
control: Processctl; # Parent waits for exact child retirement.
Workers: con 8; # Concurrent permanently assigned execution threads.
Rounds: con 512; # Private mapped copies per child, without VM quanta.


# EN: Keep mapped arrays private, then send checksum/PID through a real channel.
# id selects initial state; ready/start/done are borrowed shared endpoints.
# RU: Хранить mapped-массивы лично и вернуть checksum/PID настоящим каналом.
# id задаёт начальное значение; ready/start/done — общие заимствованные каналы.
worker(id: int, ready, start: chan of int, done: chan of (int, int))
{
    source := array[] of {"owner-" + string id, "stable-" + string id, "third-" + string id};
    target := array[3] of string;
    value := id;
    ready <-= 1;
    <-start;
    for (at := 0; at < Rounds; at++) {
        source[0] = "owner-" + string id + "-round-" + string at;
        target[0:] = source;
        if (target[0] != source[0] || target[1] != source[1]) {
            raise "parallel mapped slice changed its private references";
        }
        target[1:] = target[:2];
        if (target[0] != source[0] || target[1] != source[0] || target[2] != source[1]) {
            raise "parallel overlapping slice lost snapshot ownership";
        }
        value += at;
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
            raise "mapped counter fixture changed real arithmetic or completion identity";
        }
        seen[owner] = 1;
        result := control->killwait(pid);
        if (result != Processctl->Retired && result != Processctl->Denied) {
            raise "mapped counter fixture did not retire its real producer";
        }
    }
    sys->print("PASS resident mapped counters workers=8 rounds=512\n");
}
