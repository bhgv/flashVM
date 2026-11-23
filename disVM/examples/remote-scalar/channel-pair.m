# Copyright (c) 2026 bhgv
# EN: One caller-owned channel rented to two residents on the provider.
# RU: Один канал клиента, выданный двум процессам на провайдере.
Channelpair: module {
    Item: adt {
        label: string;
    };
    Payload: type (string, array of byte, ref Item, ref Item);
    integer: fn(wire: chan of int, unused: chan of string, probe: int): int;
    managed: fn(wire: chan of Payload, unused: chan of int): int;
};
