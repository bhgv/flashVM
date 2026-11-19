# unibus-9p-bridge

Copyright (c) 2026 bhgv

[English](README.md) · [Русский](README.ru.md) · [Index](../README.md)

## Purpose and design

Two independent clients share S-record stdin/stdout and each owns a separate loopback 9P backend connection. RX dispatch never waits for backend replies. Worker replies are serialized as complete outer records. This optional POSIX application does not link libstyx.

## Build and run

```sh
cmake -S . -B build -DUNIBUS_QUEUES=ON -DBUILD_TESTING=ON
cmake --build build -j
python3 examples/bridge/check.py build/examples/unibus-9p-bridge
ctest --test-dir build -R ^unibus-gateway$ --output-on-failure
```

## Parameters and results

| Argument | Values | Meaning |
| --- | --- | --- |
| argv1 | 1..65535 | Client7 loopback TCP backend port |
| argv2 | 1..65535 | Client8 loopback TCP backend port |
| stdin/stdout | binary S-record stream | Shared RX / serialized replies |

Launch through binary pipes. check.py creates the backends and drives a valid exchange; do not type text on stdin. Partial-record EOF exits with status 2.

Outer tags 7/8 bind example.9p.v1 version 1; inner tag 1 carries raw 9P and inner tag 2 reports backend failure. Frame limit is 8192 bytes. Each client has one backend RPC in flight: same-client concurrent Tflush is outside this teaching gateway. Client7 failure does not cancel client8 requests. Writes are never automatically replayed. Socket timeout is 3 seconds per I/O.

## Source and reference

- [Implementation](../bridge/main.c)
- [API](../../docs/index.md)
- [Protocol](../../docs/protocol.md)
- [Tutorial](../../docs/tutorial.md)
- [Main project](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)
