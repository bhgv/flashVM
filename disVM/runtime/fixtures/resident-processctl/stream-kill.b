# Copyright (c) 2026 bhgv
# EN: Kill one mapped ALT sender while another keeps using the same channel.
# RU: Завершить mapped-отправителя ALT, сохранив передачу второго по тому же каналу.
implement StreamKill;
include "sys.m";
include "processctl.m";

StreamKill: module { init: fn(); };
sys: Sys; # Read-only interface published before spawning any parent.
control: Processctl; # Read-only process control, with TLS caller identity.
Parents: con 8; # Independent concurrent traffic/control families.
Count: con 128; # Exact number of surviving peer messages per channel.


# EN: Immutable transferred graph, with producer identity and checked nested data.
# RU: Неизменяемый передаваемый граф с номером отправителя и вложенными данными.
Packet: adt {
    kind: int; # One marks the victim; two marks the surviving peer.
    sequence: int; # Monotonic sequence local to its sender.
    cells: array of int; # Owned nested array initialized before publication.
};


# EN: Send through duplicate ALT guards. Kind one waits for external termination;
# kind two sends Count packets and then acknowledges completion on done.
# data/ids/done are shared channels, each packet remains immutable after send.
# RU: Отправлять через повторные guards ALT. Вид один ждёт внешнего завершения,
# вид два посылает Count пакетов и подтверждает done. Каналы общие, графы
# после публикации не изменяются.
produce(kind: int, data: chan of ref Packet, ids, done: chan of int)
{
    if (kind == 1) {
        ids <-= control->pid();
    }
    for (sequence := 0; kind == 1 || sequence < Count; sequence++) {
        value := ref Packet(kind, sequence, array[8] of int);
        for (cell := 0; cell < len value.cells; cell++) {
            value.cells[cell] = kind * 100000 + sequence * 8 + cell;
        }
        alt {
            data <-= value => {}
            data <-= value => {}
        }
    }
    done <-= 1;
}


# EN: Check a received graph and its exact sender-local FIFO sequence.
# value is a borrowed reference; next is private to the receiving parent.
# RU: Проверить граф и точный порядковый номер отправителя. value заимствован,
# next принадлежит только принимающему родителю.
verify(value: ref Packet, next: array of int)
{
    if (value == nil || value.kind < 1 || value.kind > 2 || len value.cells != 8 ||
        value.sequence != next[value.kind]) {
        raise "stream kill changed packet identity or FIFO";
    }
    next[value.kind]++;
    for (cell := 0; cell < len value.cells; cell++) {
        if (value.cells[cell] != value.kind * 100000 + value.sequence * 8 + cell) {
            raise "stream kill corrupted a live mapped graph";
        }
    }
}


# EN: Retire the victim after both senders made progress, then drain the peer.
# capacity selects buffered/unbuffered traffic. All state is private to this parent.
# RU: Завершить жертву после продвижения обоих отправителей, затем дочитать соседа.
# capacity задаёт размер буфера. Всё состояние проверки приватно для родителя.
wave(capacity: int)
{
    data := chan[capacity] of ref Packet;
    ids := chan of int;
    done := chan of int;
    next := array[3] of int;
    next[1] = next[2] = 0;
    spawn produce(1, data, ids, done);
    victim := <-ids;
    spawn produce(2, data, ids, done);
    while (next[1] < 8 || next[2] < 8) {
        verify(<-data, next);
    }
    if (control->killwait(victim) != Processctl->Retired ||
        control->killwait(victim) != Processctl->Denied) {
        raise "stream victim did not retire exactly once";
    }
    trailing := 0;
    while (next[2] < Count) {
        value := <-data;
        verify(value, next);
        if (value.kind == 1) {
            trailing++;
        }
    }
    if (<-done != 1) {
        raise "surviving stream sender did not complete";
    }
    for (;;) {
        value: ref Packet;
        received := 0;
        alt {
            value = <-data => received = 1;
            * => {}
        }
        if (!received) {
            break;
        }
        verify(value, next);
        if (value.kind != 1) {
            raise "surviving sender published more than Count packets";
        }
        trailing++;
    }
    if (trailing > capacity || next[2] != Count) {
        raise "retired sender retained a registration or produced new traffic";
    }
}


# EN: Keep one controller bound to all three channel waves; report once at exit.
# done is shared only for the final scalar acknowledgement.
# RU: Сохранить одного владельца на три серии каналов и ответить один раз.
# done используется только для финального scalar-подтверждения.
parent(done: chan of int)
{
    wave(0);
    wave(1);
    wave(3);
    done <-= 1;
}


# EN: Start concurrent families and wait for all channel/termination checks.
# RU: Запустить семьи параллельно и дождаться всех проверок передачи/завершения.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    done := chan of int;
    for (id := 0; id < Parents; id++) {
        spawn parent(done);
    }
    for (completed := 0; completed < Parents; completed++) {
        if (<-done != 1) {
            raise "stream parent did not finish";
        }
    }
    sys->print("PASS resident stream kill peers=%d victims=%d\n", Parents * 3, Parents * 3);
}
