# Copyright (c) 2026 bhgv
# EN: Import shared by the caller and the original wide owner.
# RU: Общий импорт клиента и исходного владельца wide.
Wide: module {
    echo: fn(value: big): big; # Stateful signed 64-bit export.
};

WideSet: module {
    count: fn(delta: int): int; # Update the same owner state through an int call.
    echo: fn(value: big): big; # Update and query that state through a big call.
};
