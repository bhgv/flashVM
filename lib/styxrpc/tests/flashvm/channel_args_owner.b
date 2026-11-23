# Copyright (c) 2026 bhgv
# EN: Operate on FlashVM's call-scoped argument channels with ordinary Limbo syntax.
# RU: Операции над контекстными аргументами-каналами FlashVM штатным синтаксисом Limbo.
implement Arguments;
include "channel_args.m";


# EN: Check alias and nil admission, then exchange typed values on both arguments.
# RU: Проверить алиас и nil, затем обменяться значениями через оба аргумента.
exchange(ports: array of chan of int, messages: chan of string): int
{
    if (len ports != 4 || ports[0] != ports[1] || ports[2] != nil || ports[0] == ports[3]) {
        raise "argument channel identity mismatch";
    }
    value: int;
    alt {
        spurious := <-ports[3] => raise "empty argument selected";
        value = <-ports[0] => ;
    }
    ports[1] <-= value + 1;
    word := <-messages;
    if (word != "a\0б") {
        raise "counted string did not reach DisVM";
    }
    messages <-= word + "!";
    return value;
}


# EN: Report a remote failure after a real successful channel send.
# RU: Выдать удалённую ошибку после успешной отправки в канал.
reject(port: chan of int)
{
    port <-= 99;
    raise "after channel send";
}


# EN: Return a new provider-owned channel while using the caller's argument.
# RU: Вернуть новый канал провайдера, используя аргумент вызывающего.
result(port: chan of int): chan of int
{
    value := <-port;
    owned := chan[1] of int;
    owned <-= value + 2;
    return owned;
}


# EN: Make a second resident owner send to an unbuffered argument channel.
# RU: Запустить второго резидентного владельца для отправки в синхронный аргумент.
send(port, done: chan of int)
{
    port <-= -19;
    done <-= 1;
}


# EN: Pair two native Dis operations on the same caller-owned unbuffered channel.
# RU: Сопоставить две нативные операции Dis на синхронном канале вызывающего.
relay(port: chan of int): int
{
    done := chan[1] of int;
    spawn send(port, done);
    value := <-port;
    if (<-done != 1) {
        raise "unbuffered send did not complete";
    }
    return value;
}


# EN: Leave an empty argument receive pending until the forward caller retires its fid.
# RU: Оставить приём из пустого аргумента до закрытия fid вызывающим.
wait(port: chan of int): int
{
    return <-port;
}
