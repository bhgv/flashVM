# Copyright (c) 2026 bhgv
# EN: Return typed channels that can wait in either direction on the native bus.
# RU: Возвращать типизированные каналы, ожидающие в любом направлении на нативной шине.
implement Library;

Library: module {
    queue: fn(seed: int): chan of int;
};


# EN: Return an independent one-slot channel, initially full if seed is nonzero.
# RU: Вернуть отдельный канал на один элемент, заполненный при ненулевом seed.
queue(seed: int): chan of int
{
    result := chan[1] of int;
    if (seed) {
        result <-= seed;
    }
    return result;
}
