# Copyright (c) 2026 bhgv
# EN: Contended ALT send/receive with duplicate guards and nil array entries.
# RU: Конкурирующие ALT отправки/приёма с повторными guards и nil в массивах.
implement ResidentAlt;
include "sys.m";

ResidentAlt: module { init: fn(); };
sys: Sys; # Immutable interface shared by resident processes.
Senders: con 16; # Independent producer OS threads per wave.
Readers: con 4; # Competing ALT consumers per wave.
Rounds: con 128; # Distinct payloads from each sender.


# EN: Send each id/sequence once through duplicate guards; signal done at exit.
# Channel arguments are shared references, locals belong to this process.
# RU: Отправить id/sequence ровно один раз через повторные guards; уведомить done.
# Каналы — общие ссылки, локальные переменные принадлежат процессу.
produce(id: int, wire: chan of int, done: chan of int)
{
    for (sequence := 0; sequence < Rounds; sequence++) {
        selected := 0;
        value := id * Rounds + sequence;
        alt {
            wire <-= value => selected = 1;
            wire <-= value => selected = 2;
        }
        if (selected != 1 && selected != 2) {
            raise "ALT sender did not select exactly one branch";
        }
    }
    done <-= 1;
}


# EN: Receive a fixed share through aliased wire entries; report value and index.
# Wires and reports are shared, result locals remain private until their send.
# RU: Принять долю сообщений через повторные wire; передать значение и индекс.
# wires/reports — общие ссылки, результаты локальны до отправки.
consume(wires: array of chan of int, reports: chan of (int, int), done: chan of int)
{
    for (round := 0; round < Senders * Rounds / Readers; round++) {
        result: (int, int);
        alt {
            result = <-wires => {}
        }
        reports <-= result;
    }
    done <-= 1;
}


# EN: Run one capacity with all producers/consumers resident; verify conservation.
# capacity selects unbuffered or buffered wires. Own arrays until all done replies.
# RU: Запустить производителей/читателей с capacity, проверить сохранение данных.
# capacity задаёт буфер wire. Массивы живут до всех ответов done.
wave(capacity: int)
{
    wires := array[Senders * 2 + 1] of chan of int;
    reports := chan of (int, int);
    done := chan of int;
    for (id := 0; id < Senders; id++) {
        wire := chan[capacity] of int;
        wires[id * 2 + 1] = wire;
        wires[id * 2 + 2] = wire;
        spawn produce(id, wire, done);
    }
    for (reader := 0; reader < Readers; reader++) {
        spawn consume(wires, reports, done);
    }
    seen := array[Senders * Rounds] of int;
    for (zero := 0; zero < len seen; zero++) {
        seen[zero] = 0;
    }
    for (received := 0; received < len seen; received++) {
        (slot, value) := <-reports;
        if (slot < 1 || slot >= len wires || value < 0 || value >= len seen ||
            (slot - 1) / 2 != value / Rounds || seen[value]) {
            sys->print("ALT mismatch capacity=%d received=%d slot=%d value=%d\n", capacity, received, slot, value);
            raise "ALT selected wrong guard or duplicated payload";
        }
        seen[value] = 1;
    }
    for (completed := 0; completed < Senders + Readers; completed++) {
        if (<-done != 1) {
            raise "ALT worker did not complete";
        }
    }
    for (index := 0; index < len seen; index++) {
        if (seen[index] != 1) {
            raise "ALT lost payload";
        }
    }
    untouched := (73, 91);
    alt {
        untouched = <-wires => raise "ALT left data or a stale sender";
        * => {}
    }
    (unchangedslot, unchangedvalue) := untouched;
    if (unchangedslot != 73 || unchangedvalue != 91) {
        raise "ALT default changed destination";
    }
    sys->print("PASS resident ALT capacity=%d payloads=%d\n", capacity, len seen);
}


# EN: Initialize shared Sys before spawn, then run three buffer sizes serially.
# No arguments/result; a VM exception reports any broken transport invariant.
# RU: Загрузить Sys до spawn, проверить три размера буфера последовательно.
# Без параметров/результата; нарушение передачи вызывает исключение VM.
init()
{
    sys = load Sys Sys->PATH;
    wave(0);
    wave(1);
    wave(3);
    sys->print("PASS resident ALT stress\n");
}
