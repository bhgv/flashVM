# Copyright (c) 2026 bhgv
# EN: Compiled contract for nested and returned ref identities.
# RU: Компилированный контракт вложенных и возвращаемых ref.
Graphs: module {
    Leaf: adt {
        count: int;
    };
    Box: adt {
        inner: ref Leaf;
        untouched: array of byte;
    };
    TupleBox: adt {
        pair: (int, ref Leaf);
        untouched: array of byte;
    };
    nested: fn(box: ref Box): ref Leaf;
    replace: fn(box: ref Box, value: int): ref Leaf;
    make: fn(value: int): ref Leaf;
    fail: fn(box: ref Box): ref Leaf;
    fan: fn(values: array of ref Leaf): ref Leaf;
    tupleedit: fn(box: ref TupleBox, value: int): ref Leaf;
};
