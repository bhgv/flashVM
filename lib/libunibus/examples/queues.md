# unibus-queues

Copyright (c) 2026 bhgv

[English](queues.md) · [Русский](queues.ru.md) · [Index](README.md)

## Purpose and design

A pthread consumer waits only for client 7 while the dispatcher delivers client 8 immediately. The barrier establishes concurrent lifetime without sleeps. Main then delivers client 7, joins the consumer and closes/frees both engines.

## Build and run

```sh
cmake -S . -B build -DUNIBUS_QUEUES=ON -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/unibus-queues
ctest --test-dir build -R ^unibus-queues$ --output-on-failure
```

## Parameters and results

No command-line arguments. Exit 0 means all example checks passed; exit 1 reports failure and stderr identifies the stage. Stdout is a PASS diagnostic, not an arbitrary binary transport.

[styxordercreate](../docs/reference/styxordercreate.md) uses a maximum message of 256 and window 16; client7 wait is bounded to 5000 ms. The barrier confirms worker startup, not exact wait admission. Setup failures release every constructed engine/barrier. After worker creation, receiver close precedes join; ready records remain drainable. unibus-example-failures injects four partial-initialization failures. Unexpected join failure terminates this teaching process through _Exit: state borrowed by an unretired worker cannot be reclaimed safely. An embedding application must choose its own fatal-failure policy. These are example settings, not policy for every client. Historical styxorder C names preserve source compatibility without a libstyx dependency.

## Source and reference

- [Implementation](queues.c)
- [API](../docs/index.md)
- [Protocol](../docs/protocol.md)
- [Tutorial](../docs/tutorial.md)
- [Main project](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
