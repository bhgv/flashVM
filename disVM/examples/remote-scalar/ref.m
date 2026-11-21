# Copyright (c) 2026 bhgv
# EN: A scalar-only mutable argument shared by the caller and original Dis export.
# RU: Изменяемый скалярный аргумент клиента и исходного экспорта Dis.

Cell: adt {
    narrow: int; # Signed narrow field.
    wide: big;   # Signed full-width field.
};

Refexample: module {
    revise: fn(value: ref Cell): (int,big); # Return the call number and original wide field.
};

Refpairexample: module {
    exchange: fn(first, second: ref Cell): (int,big); # Aliasing is observed inside the call.
};
