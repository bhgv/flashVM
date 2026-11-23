# Copyright (c) 2026 bhgv
# EN: Exercise direct Limbo sends and receives on the remote owner's arguments.
# RU: Проверить обычные отправку и приём Limbo на аргументах удалённого владельца.
implement Chainchannel;
include "chain-channel.m";


# EN: Run a child using inherited channel authority, then return its outcome.
# RU: Запустить дочерний поток с унаследованным доступом к каналам и вернуть итог.
typed(input, output: chan of Payload, wide: chan of big): int
{
    result := chan of int;
    spawn typed_worker(input, output, wide, result);
    return <-result;
}


# EN: Select a typed payload, verify identity inside it, and send the altered value.
# RU: Выбрать типизированное значение, проверить общую ссылку и отправить изменения.
typed_worker(input, output: chan of Payload, wide: chan of big, result: chan of int)
{
    passed := 0;
    {
        alt {
            (text, values, first, second) := <-input =>
                if (text != "Юнибас" || len values != 900 || values[899] != 1798 ||
                    first == nil || first != second || first.number != 37) {
                    raise "wrong typed channel payload";
                }
                values[899] = -18;
                first.number = 91;
                output <-= ("✓", values, first, second);
            other := <-wide =>
                raise "unexpected wide guard";
        }
        if (<-wide != big 9223372036854775807) {
            raise "wrong channel scalar";
        }
        passed = 1;
    } exception {
        * => passed = 0;
    }
    result <-= passed;
}


# EN: Receive a byte, real and nil string, then send a new string.
# RU: Принять byte, real и строку nil, затем отправить новую строку.
scalars(tiny: chan of byte, fraction: chan of real, text: chan of string): int
{
    if (<-tiny != byte 251 || <-fraction != 1.25 || <-text != nil) {
        return 0;
    }
    text <-= "✓";
    return 1;
}


# EN: Return the existing input, nil, or a new unexported channel according to mode.
# RU: Вернуть исходный вход, nil или новый неопубликованный канал согласно mode.
channelresult(input: chan of int, mode: int): chan of int
{
    case mode {
        0 => return input;
        1 => return nil;
        * => return chan of int;
    }
}


# EN: Verify alias identity and use input and the tuple of output/alias according to op.
# Return the computed int after completion, or raise on unexpected arguments.
# RU: Проверить совпадение ссылок и использовать input и кортеж output/alias по op.
# Возвращает int после завершения либо бросает ошибку неверных аргументов.
exchange(op: int, input: chan of int, ports: (chan of int, chan of int)): int
{
    (output, alias) := ports;
    if (alias != input) {
        raise "channel aliases lost";
    }
    case op {
        0 =>
            value := <-input;
            output <-= value + 17;
            return value - 17;
        1 =>
            output <-= -17;
            return 23;
        2 =>
            result := chan[1] of int;
            spawn pending(input, output, result);
            return <-result;
        3 =>
            return input == nil && output == nil;
        4 =>
            alt {
                value := <-input =>
                    return value + 1;
                value := <-output =>
                    return value - 1;
            }
        5 =>
            alt {
                output <-= -29 =>
                    return 31;
                * =>
                    return 37;
            }
        6 or 7 =>
            local := chan[1] of int;
            if (op == 6) {
                local <-= 77;
            }
            alt {
                value := <-local =>
                    return value;
                value := <-input =>
                    return value + 1;
                * =>
                    return -5;
            }
    }
    raise "invalid channel example operation";
}


# EN: Signal entry, then wait on an inherited proxy until the caller cancels.
# RU: Сообщить о входе, затем ждать на унаследованном прокси до отмены.
pending(input, output, result: chan of int)
{
    value := -91;
    {
        output <-= -23;
        value = <-input;
    } exception {
        * => {}
    }
    result <-= value;
}
