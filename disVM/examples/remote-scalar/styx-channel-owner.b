# Copyright (c) 2026 bhgv
# EN: Exercise direct Limbo sends and receives on the remote owner's arguments.
# RU: Проверить обычные отправку и приём Limbo на аргументах удалённого владельца.
implement Chainchannel;
include "chain-channel.m";


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
            output <-= -23;
            return <-input;
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
    }
    raise "invalid channel example operation";
}
