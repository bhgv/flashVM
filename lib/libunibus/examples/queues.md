# unibus-queues

Copyright (c) 2026 bhgv

[English](queues.md) · [Русский](queues.ru.md) · [Index](README.md)

## Purpose and design

A pthread consumer waits only for client 7 while the dispatcher delivers client 8. The barrier establishes concurrent lifetime without sleeps, not exact wait admission. Main then delivers client 7 and retires the worker and resources.

## Build and run

```sh
cmake -S . -B build -DUNIBUS_QUEUES=ON -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/unibus-queues
ctest --test-dir build -R '^unibus-queues$' --output-on-failure
```

## Parameters and results

No command-line arguments. Exit 0 means the exact expected result; exit 1 reports failure and stderr identifies the stage. Stdout is a PASS diagnostic, not binary carrier bytes.

[styxordercreate](../docs/reference/styxordercreate.md) selects maximum message 256 and window 16. Client 7 wait is bounded to 5000 ms; these are example settings, not policy for every client. Historical styxorder C names preserve source compatibility without creating a libstyx dependency.

Setup failures release all constructed engines/barriers. After worker creation, receiver close precedes join; ready records remain drainable. unibus-example-failures injects four partial-initialization failures. Unexpected join failure terminates the teaching process through _Exit: borrowed worker state cannot be reclaimed before retirement is proven. An embedding application chooses its own fatal-failure policy.

## Source and reference

- [Implementation](queues.c)
- [API](../docs/index.md)
- [Protocol](../docs/protocol.md)
- [Tutorial](../docs/tutorial.md)
- [Main project](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
