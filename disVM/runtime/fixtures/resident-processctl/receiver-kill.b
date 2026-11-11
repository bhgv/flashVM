# Copyright (c) 2026 bhgv
# EN: Retire a receiving process while a peer drains the same mapped channel.
# RU: Завершить принимающий процесс, пока другой читатель дочитывает общий канал.
implement ReceiverKill;
include "sys.m";
include "processctl.m";

ReceiverKill: module { init: fn(); };
sys: Sys; # Immutable interface initialized before any worker starts.
control: Processctl; # Read-only module with caller identity in TLS.
Parents: con 8; # Concurrent independent channel/termination families.
Count: con 136; # Total produced packets, including eight initial acknowledgements.
CancelMode: con 0; # Test driver selects cancellation instead of forced termination.


# EN: Immutable graph retained by channel, consumer and reporting parent.
# RU: Неизменяемый граф во владении канала, читателя и проверяющего родителя.
Packet: adt {
    sequence: int; # Unique ordinal; minus one is the final end marker.
    cells: array of int; # Eight checked values for ordinary packets.
};


# EN: Publish all Count packets, then acknowledge the exact count.
# data/produced are shared; every packet is immutable after publication.
# RU: Опубликовать Count пакетов и подтвердить их количество.
# Каналы общие; каждый пакет неизменяем после публикации.
produce(data: chan of ref Packet, produced: chan of int)
{
    for (sequence := 0; sequence < Count; sequence++) {
        value := ref Packet(sequence, array[8] of int);
        for (cell := 0; cell < len value.cells; cell++) {
            value.cells[cell] = sequence * 8 + cell;
        }
        data <-= value;
    }
    produced <-= Count;
}


# EN: Receive through duplicate/nil array guards and report one packet at a time.
# victim announces its PID. Reports are unbuffered: termination can discard at
# most one received packet whose report has not yet been accepted by the parent.
# RU: Принимать через массив с повторными/nil guards и сообщать по одному пакету.
# victim публикует PID. reports без буфера: завершение может отбросить не более
# одного принятого пакета, отчёт о котором родитель ещё не подтвердил.
consume(victim: int, data, reports: chan of ref Packet, ids, done: chan of int)
{
    if (victim) {
        ids <-= control->pid();
    }
    choices := array[3] of chan of ref Packet;
    choices[1] = data;
    choices[2] = data;
    {
        for (;;) {
            result: (int, ref Packet);
            alt {
                result = <-choices => {}
            }
            (slot, value) := result;
            if (slot != 1 && slot != 2) {
                raise "receiver kill selected a nil guard";
            }
            if (value == nil) {
                raise "receiver kill lost a mapped packet";
            }
            if (value.sequence == -1) {
                if (victim) {
                    raise "retired receiver consumed the end marker";
                }
                done <-= 1;
                return;
            }
            reports <-= value;
        }
    } exception {
        "channel wait cancelled" => {
            if (!CancelMode || !victim) {
                raise "unexpected receiver cancellation";
            }
        }
    }
}


# EN: Validate the borrowed graph and record exactly one delivery in private seen.
# RU: Проверить заимствованный граф и отметить единственную доставку в seen.
verify(value: ref Packet, seen: array of int)
{
    if (value == nil || value.sequence < 0 || value.sequence >= Count ||
        len value.cells != 8 || seen[value.sequence]) {
        raise "receiver kill duplicated or damaged a packet";
    }
    for (cell := 0; cell < len value.cells; cell++) {
        if (value.cells[cell] != value.sequence * 8 + cell) {
            raise "receiver kill freed a graph still owned by a peer";
        }
    }
    seen[value.sequence] = 1;
}


# EN: Start the second reader after eight victim reports, then retire the victim.
# capacity selects the buffer. Drain reports while sending the final marker to
# avoid blocking the sole reader behind the parent's own send operation.
# RU: После восьми отчётов жертвы запустить соседа и завершить жертву.
# capacity задаёт буфер. Отчёты принимаются и при отправке конечной метки,
# чтобы родитель не заблокировал единственного читателя собственной отправкой.
wave(capacity: int)
{
    data := chan[capacity] of ref Packet;
    victimreports := chan of ref Packet;
    peerreports := chan of ref Packet;
    ids := chan of int;
    produced := chan[1] of int;
    done := chan of int;
    seen := array[Count] of int;
    for (clear := 0; clear < len seen; clear++) {
        seen[clear] = 0;
    }
    spawn consume(1, data, victimreports, ids, done);
    victim := <-ids;
    spawn produce(data, produced);
    for (initial := 0; initial < 8; initial++) {
        verify(<-victimreports, seen);
    }
    spawn consume(0, data, peerreports, ids, done);
    if (CancelMode) {
        for (;;) {
            result := control->cancelwait(victim);
            if (result == Processctl->Cancelled) {
                break;
            }
            if (result != Processctl->NotWaiting) {
                raise "receiver cancellation lost its child";
            }
            sys->sleep(1);
        }
        retired := control->killwait(victim);
        if (retired != Processctl->Retired && retired != Processctl->Denied) {
            raise "cancelled receiving victim did not retire";
        }
    } else {
        if (control->killwait(victim) != Processctl->Retired ||
            control->killwait(victim) != Processctl->Denied) {
            raise "receiving victim did not retire exactly once";
        }
    }
    complete := 0;
    while (!complete) {
        value: ref Packet;
        total: int;
        alt {
            value = <-peerreports => verify(value, seen);
            total = <-produced => {
                if (total != Count) {
                    raise "producer did not finish after receiver retirement";
                }
                complete = 1;
            }
        }
    }
    ending := ref Packet(-1, nil);
    sent := 0;
    while (!sent) {
        value: ref Packet;
        alt {
            data <-= ending => sent = 1;
            value = <-peerreports => verify(value, seen);
        }
    }
    finished := 0;
    while (!finished) {
        value: ref Packet;
        result: int;
        alt {
            value = <-peerreports => verify(value, seen);
            result = <-done => {
                if (result != 1) {
                    raise "surviving receiver did not finish";
                }
                finished = 1;
            }
        }
    }
    missing := 0;
    for (index := 0; index < len seen; index++) {
        missing += !seen[index];
    }
    if (missing > 1) {
        raise "receiver retirement lost more than its one pending report";
    }
    alt {
        <-victimreports => raise "retired receiver retained a pending report";
        <-data => raise "channel retained data after the end marker";
        * => {}
    }
}


# EN: Keep one parent bound across all buffer geometries, report once.
# RU: Сохранить одного родителя для всех размеров буфера, ответить один раз.
parent(done: chan of int)
{
    wave(0);
    wave(1);
    wave(3);
    done <-= 1;
}


# EN: Exercise eight families concurrently and await all completion messages.
# RU: Запустить восемь семей параллельно и дождаться всех сообщений завершения.
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
            raise "receiver kill parent did not complete";
        }
    }
    if (CancelMode) {
        sys->print("PASS resident receiver cancel peers=%d victims=%d\n", Parents * 3, Parents * 3);
    } else {
        sys->print("PASS resident receiver kill peers=%d victims=%d\n", Parents * 3, Parents * 3);
    }
}
