# Copyright (c) 2026 bhgv
# EN: Pure provider interface shared by compiler and executing residents.
# RU: Чистый интерфейс provider для компилирующих и исполняющих потоков.
JitProvider: module {
    run: fn(value: int): int; # Return a private arithmetic checksum.
};
