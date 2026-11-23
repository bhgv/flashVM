# Copyright (c) 2026 bhgv
# EN: Compiled contract for FlashVM-owned FIFO and rendezvous channels.
# RU: Компилированный контракт буферизованных и синхронных каналов FlashVM.
Channelservice: module {
    port: fn(seed: string): chan of string;
    rendezvous: fn(): chan of int;
    tree: fn(seed: int): chan of array of chan of int;
    badseed: fn(seed: string): chan of int;
};
