# Copyright (c) 2026 bhgv
# EN: B invokes C's library from inside A's active library call.
# RU: B вызывает библиотеку C внутри библиотечного вызова от A.
implement Chainfd;
include "chain-fd.m";


# EN: Use the same typed interface on the library mounted from C.
# RU: Использовать тот же типизированный интерфейс библиотеки с C.
forward(input, output: int): int
{
    next := load Chainfd "/remote-libs/mounted/chain-fd.dis";
    if (next == nil) {
        raise "nested library could not load C";
    }
    return next->forward(input, output);
}
