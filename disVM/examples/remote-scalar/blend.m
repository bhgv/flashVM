# Copyright (c) 2026 bhgv
# EN: A library interface with three arguments and a nested return tuple.
# RU: Интерфейс библиотеки с тремя аргументами и вложенным кортежем результата.
BlendWord: type int; # Resolved by the ordinary compiler.

Blend: module {
    blend: fn(seed: BlendWord, pair: (BlendWord,big), delta: big): (BlendWord,(BlendWord,big)); # Three positions on the wire.
};
