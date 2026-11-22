# Copyright (c) 2026 bhgv
# EN: Original Dis implementation exercised through the mounted library.
# RU: Исходная реализация Dis для вызовов через смонтированную библиотеку.
implement Managed;
include "managed.m";
# EN: Mutate the locally decoded byte array and return it with the decorated text.
# RU: Изменить локальный массив байтов и вернуть его вместе с дополненной строкой.
echo(text: string, bytes: array of byte): (string, array of byte)
{
    if (len bytes < 3 || bytes[0] != byte 42) {
        raise "invalid test argument";
    }
    bytes[1] = byte 99;
    return (text + "!", bytes);
}
# EN: Complete without a value or raise the requested exception.
# RU: Завершиться без значения либо вызвать исключение по запросу.
accept(text: string)
{
    if (text != "ok") {
        raise "rejected";
    }
}


# EN: Change the aliased ADT once, touch an unchanged field, or raise after a write.
# RU: Изменить общий ADT, записать прежнее значение либо вызвать исключение после записи.
update(mode: string, left, right, untouched: ref Cell): string
{
    if (left != right) {
        raise "remote aliases diverged";
    }
    untouched.text = untouched.text;
    if (mode == "noop") {
        return untouched.text;
    }
    if (mode == "grow") {
        grown := array[len left.bytes + 1] of byte;
        grown[0:] = left.bytes;
        grown[len grown - 1] = byte 77;
        left.bytes = grown;
        return right.text;
    }
    if (mode == "shrink") {
        left.bytes = left.bytes[:len left.bytes - 1];
        return right.text;
    }
    left.text = left.text + "!";
    if (mode != "text") {
        right.bytes[1] = byte 91;
        right.bytes[len right.bytes - 1] = byte 92;
    }
    if (mode == "fail") {
        raise "discard ref updates";
    }
    return right.text;
}
