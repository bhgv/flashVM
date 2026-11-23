# Copyright (c) 2026 bhgv
# EN: Remote mixed selections and channels whose origin owns them across calls.
# RU: Удалённый смешанный выбор и каналы, которыми источник владеет между вызовами.
Chainchannellifetime: module {
    mixed: fn(mode: int, remote, entered: chan of int): (int, int);
    buffered: fn(value: int): (chan of int, chan of int);
    worker: fn(start: int): (chan of int, chan of int);
};
