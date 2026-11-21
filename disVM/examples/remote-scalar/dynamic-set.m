# Copyright (c) 2026 bhgv
# EN: Scalar-only exports admitted by their compiled types.
# RU: Скалярные экспорты, допускаемые по скомпилированным типам.

Dynamicset: module {
    status: fn(): int; # Empty argument list.
    adjust: fn(delta: big); # Scalar argument with no return.
    project: fn(value: (big,int)): (big,int); # Two cells in one tuple.
    fold: fn(seed: int, value: (big,int)): (int,(int,big)); # Three cells with nested return.
    quad: fn(value: (big,int), step: int, tail: big): (big,(int,int),big); # Four cells at either end.
    fractional: fn(input: (real,int), shift: real): (int,(real,real));
    clear: fn(); # Empty arguments and return.
    tally: fn(amount: int): int; # Existing fixed int profile on the same instance.
};
