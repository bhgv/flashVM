# Lazy paginated HTTP hierarchy

Copyright (c) 2026 bhgv

[Русский](README.ru.md) · [Index](../README.md)

## Purpose and choice

Every stat/list query runs the HTTP adapter with the selected stable object ID and one immediate-child page. Startup does not fetch the tree. The supplied real HTTP fixture computes a synthetic hierarchy with billions of nodes without storing it. It is a reproducible test, not real geographic information. Replace the JSON service with an Internet/database adapter for your actual data.

## Build and run

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON  # Hosted example targets / Нативные примеры
cmake --build build -j  # Build the library and examples / Собрать библиотеку и примеры
python3 examples/http_source.py 8080  # Run in another terminal / Запустить в другом терминале
./build/examples/styx-http 5640 http://127.0.0.1:8080/node  # Keep running in terminal1 / Оставить в терминале1
```

```sh
./build/examples/styx-cat 5640 Exampleland/city-2/district-3/street-4/house-5/room-6/info  # Read through original native 9P client / Читать нативным 9P-клиентом
ctest --test-dir build -R '^styx-http$' --output-on-failure  # Independent wire acceptance / Независимая wire-проверка
```

## Parameters

| Parameter | Values | Meaning |
| --- | --- | --- |
| argv1 | `0..65535`, default `0` | TCP port;0 chooses an ephemeral port. First stdout line is the actual port. |
| argv2 | HTTP(S) endpoint URL | Operator supplies the source explicitly; clients do not supply host paths. |
| names | `country/city/district/street/house/room/info` | Namespace of this example. |
| file sample | `Exampleland/city-2/district-3/street-4/house-5/room-6/info` | Expected text: `Synthetic information for room-6` |

## Request flow

Twalk resolves components into a fid. Topen validates mode and pins the object. File Tread uses its held ID and offset. Directory Tread emits only complete Dir entries for immediate children; offset counts encoded bytes. Tclunk/disconnect performs cleanup. The tree may change between requests; no walk is added before every read/write.

## Bounds and replacing the backend

CatalogRow holds a name up to127 UTF-8 bytes and demo text up to2047 bytes; a page has at most16 children. For photos/large content replace text reads with byte-stream/offset reads. 9P does not require a whole file in memory. A listing changed between pages can require rewind to offset0. Example backend calls are synchronous; production asynchronous I/O uses the deferred-read API.

The TSV demo is bounded to128 keys and rereads one flat dataset, not a recursive tree. The live demo has16 visible/held window slots. The HTTP helper bounds responses to128KiB, socket waits to3s and uses a5s process alarm; IDs must preserve exact uint64 values, without JavaScript-double rounding.

## Source and reference

- [Server callbacks](../server.c), [catalog_query / pin / unpin contract](../catalog.h), [backend implementation](../http_catalog.c).
- [styxinit](../../docs/reference/styxinit.md), [convD2M](../../docs/reference/convD2M.md), [convM2S](../../docs/reference/convM2S.md).
- [Original client](../client.c), [independent wire client](../wire_client.py), [acceptance](../check.py).
- [Main-project native Styx help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md).

## HTTP backend schema

`GET endpoint?op=list&id=0&start=0&limit=16` returns a JSON array. `op=stat` returns zero or one row. Each row has exact unsigned64 `id`/`parent`, numeric standard `mode`, UTF-8 `name`, optional text `value`. The source owns stable identities and access policy; changing a filename must not alias an unrelated held ID.

[HTTP adapter](../http_query.py) · [HTTP fixture](../http_source.py).
