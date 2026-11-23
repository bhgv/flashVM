# Copyright (c) 2026 bhgv
# EN: Compiled signatures of the hosted mailbox and its owner-side checks.
# RU: Компилированные сигнатуры хостовой очереди и проверок владельца.
Pidresult: module {
    mailbox: fn(marker: int): chan of chan of int;
    drain: fn(tick: chan of int): int;
    retired: fn(tick: chan of int): int;
};
