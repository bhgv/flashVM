# Copyright (c) 2026 bhgv
# EN: One bounded exact network exchange; managed ownership stays in its caller.
# RU: Один bounded exact network exchange; managed ownership остаётся у caller.
Proxynetio: module
{
    PATH: con "proxynetio.dis"; # Shared private wire implementation.


    # EN/RU: Initialize immutable clock/codec/stream before sharing.
    # Инициализировать immutable clock/codec/stream до sharing.
    init: fn();


    # EN: Borrow mapped request/FD owners through one absolute exchange deadline.
    # Return status/value/terminal/returned handle; never replay native effects.
    # RU: Заимствовать mapped request/FD owners до absolute exchange deadline.
    # Вернуть status/value/terminal/returned handle; без replay native effects.
    exchange: fn(input, output: ref Sys->FD, request: ref Proxynet->Request,
                 high, low, id: big, timeout: int): (int, int, int, Proxyowner->Handle);
};
