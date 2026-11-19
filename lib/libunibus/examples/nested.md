# unibus-nested

Copyright (c) 2026 bhgv

[English](nested.md) · [Русский](nested.ru.md) · [Index](README.md)

## Purpose and design

Encode tag-zero schema assignment, then feed a nested client message one byte at a time. Outer tag 7 and inner tag 1 have different scopes. This smallest example needs no allocation, threads or physical connection.

## Build and run

```sh
cmake -S . -B build -DUNIBUS_QUEUES=OFF -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/unibus-nested
ctest --test-dir build -R '^unibus-nested$' --output-on-failure
```

## Parameters and results

No command-line arguments. Exit 0 means the exact expected result; exit 1 reports failure and stderr identifies the stage. Stdout is a PASS diagnostic, not binary carrier bytes.

128/256-byte buffers are sample capacities, not protocol limits. Two table slots are supplied; one is used. Checks cover assignment, version, schema and four sensor bytes 21.5. MORE between fragments is expected; only the final byte completes this record. Keep storage alive until the borrowed payload is consumed and reset.

## Source and reference

- [Implementation](nested.c)
- [API](../docs/index.md)
- [Protocol](../docs/protocol.md)
- [Tutorial](../docs/tutorial.md)
- [Main project](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
