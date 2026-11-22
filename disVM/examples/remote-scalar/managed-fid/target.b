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
