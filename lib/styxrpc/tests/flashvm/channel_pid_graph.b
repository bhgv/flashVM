# Copyright (c) 2026 bhgv
# EN: Exercise scoped current keys and aliases within a single FlashVM reverse PID.
# RU: Проверка текущих ключей и алиасов в пределах одного встречного PID FlashVM.
implement Pidgraph;
include "channel_pid_graph.m";


# EN: Return a route receiver and an int completion channel from one module lease.
# RU: Вернуть приёмник маршрута и канал подтверждения int из одной аренды.
open(): (chan of chan of chan of chan of int, chan of int)
{
    input := chan of chan of chan of chan of int;
    result := chan of int;
    spawn exchange(input, result);
    return (input, result);
}


# EN: Pair a second native owner with a send to the nested rendezvous.
# RU: Сопоставить второго нативного владельца с отправкой во вложенный rendezvous.
read(port, done: chan of int)
{
    done <-= <-port;
}


# EN: Validate aliases, operate on both queue kinds and move a child between messages.
# RU: Проверить алиасы, обменяться через оба вида очередей и перенести дочерний канал.
exchange(input: chan of chan of chan of chan of int, result: chan of int)
{
    root := <-input;
    first := <-root;
    alias := <-root;
    second := <-root;
    fifo := <-first;
    rendezvous := <-second;
    if (first == second || first != alias || fifo == nil || rendezvous == nil || fifo == rendezvous) {
        raise "PID graph lost aliases";
    }
    fifo <-= 43;
    seed := <-fifo;
    done := chan[1] of int;
    spawn read(rendezvous, done);
    rendezvous <-= 27;
    if (<-done != 27) {
        raise "PID graph lost rendezvous";
    }
    first <-= rendezvous;
    first <-= fifo;
    first <-= rendezvous;
    first <-= nil;
    root <-= first;
    result <-= seed;
}
