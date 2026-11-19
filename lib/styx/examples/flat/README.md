# Fixed root files

Copyright (c) 2026 bhgv

[Русский](README.ru.md) · [Index](../README.md)

## Purpose and choice

The two names and contents are constant. No backend enumeration occurs beyond the requested root. This is the smallest sensor-style server; replace the temperature read with your device sampling callback.

## Build and run

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON  # Hosted example targets / Нативные примеры
cmake --build build -j  # Build the library and examples / Собрать библиотеку и примеры
./build/examples/styx-flat 5640  # Keep running in terminal1 / Оставить в терминале1
```

```sh
./build/examples/styx-cat 5640 status  # Read through original native 9P client / Читать нативным 9P-клиентом
ctest --test-dir build -R '^styx-flat$' --output-on-failure  # Independent wire acceptance / Независимая wire-проверка
```

## Parameters

| Parameter | Values | Meaning |
| --- | --- | --- |
| argv1 | `0..65535`, default `0` | TCP port;0 chooses an ephemeral port. First stdout line is the actual port. |
| argv2 | unused | Operator supplies the source explicitly; clients do not supply host paths. |
| names | `status; temperature` | Namespace of this example. |
| file sample | `status` | Expected text: `ready\n` |

## Request flow

Twalk resolves components into a fid. Topen validates mode and pins the object. File Tread uses its held ID and offset. Directory Tread emits only complete Dir entries for immediate children; offset counts encoded bytes. Tclunk/disconnect performs cleanup. The tree may change between requests; no walk is added before every read/write.

## Bounds and replacing the backend

CatalogRow holds a name up to127 UTF-8 bytes and demo text up to2047 bytes; a page has at most16 children. For photos/large content replace text reads with byte-stream/offset reads. 9P does not require a whole file in memory. A listing changed between pages can require rewind to offset0. Example backend calls are synchronous; production asynchronous I/O uses the deferred-read API.

The TSV demo is bounded to128 keys and rereads one flat dataset, not a recursive tree. The live demo has16 visible/held window slots. The HTTP helper bounds responses to128KiB, socket waits to3s and uses a5s process alarm; IDs must preserve exact uint64 values, without JavaScript-double rounding.

## Source and reference

- [Server callbacks](../server.c), [catalog_query / pin / unpin contract](../catalog.h), [backend implementation](../fixed.c).
- [styxinit](../../docs/reference/styxinit.md), [convD2M](../../docs/reference/convD2M.md), [convM2S](../../docs/reference/convM2S.md).
- [Original client](../client.c), [independent wire client](../wire_client.py), [acceptance](../check.py).
- [Main-project native Styx help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md).

[Common server walkthrough](../common.md)
