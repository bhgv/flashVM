# unibus-nested

Copyright (c) 2026 bhgv

[English](nested.md) · [Русский](nested.ru.md) · [Index](README.md)

## Purpose and design

Encode tag-zero schema assignment, then feed a nested client message one byte at a time. Outer tag 7 and inner tag 1 have different scopes. This is the smallest allocation-free example and needs no physical connection.

## Build and run

```sh
cmake -S . -B build -DUNIBUS_QUEUES=OFF -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/unibus-nested
ctest --test-dir build -R ^unibus-nested$ --output-on-failure
```

## Parameters and results

No command-line arguments. Exit 0 means all example checks passed; nonzero identifies a failure stage in source. Stdout is a PASS diagnostic, not an arbitrary binary transport.

128/256-byte buffers are sample capacities, not protocol limits. Two binding slots are provided; one is used. Checks cover tag, schema and the four bytes 21.5; MORE between fragments is expected. Do not overwrite storage while its borrowed payload is being consumed.

## Source and reference

- [Implementation](nested.c)
- [API](../docs/index.md)
- [Protocol](../docs/protocol.md)
- [Tutorial](../docs/tutorial.md)
- [Main project](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
