# Copyright (c) 2026 bhgv
# EN: Interface shared by the ordinary client and the original remote implementation.
# RU: Общий интерфейс обычного клиента и исходной удалённой реализации.
Managed: module {
    echo: fn(text: string, bytes: array of byte): (string, array of byte);
    Cell: adt {
        text: string;
        bytes: array of byte;
    };
    update: fn(mode: string, left, right, untouched: ref Cell): string;
    Item: type (byte, string);
    Nested: adt {
        items: array of Item;
        rows: array of array of byte;
    };
    nested: fn(names: array of string, state: ref Nested): (array of string, array of Item);
    reference: fn(state: ref Cell): (ref Cell, ref Cell, ref Cell, ref Cell);
    accept: fn(text: string);
};
