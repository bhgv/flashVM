# Copyright (c) 2026 bhgv
# EN: Shared compiled Limbo contract for aliasing and atomic managed write-back.
# RU: Общий контракт Limbo для алиасов и атомарной обратной записи.
Refs: module {
    Cell: adt {
        text: string;
        bytes: array of byte;
    };
    change: fn(text: string, left, right, other: ref Cell): string;
    reject: fn(text: string, left, right, other: ref Cell): string;
};
