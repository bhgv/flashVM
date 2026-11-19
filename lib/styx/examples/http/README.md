# Dynamic namespace from an HTTP source

Copyright (c) 2026 bhgv

[English](README.md) · [Русский](README.ru.md) · [Examples](../README.md)

## What and why

This example exposes a lazy hierarchy as an ordinary 9P server: countries, cities, districts, streets, houses, rooms and info files. The HTTP source returns metadata for one ID or one page of immediate children. It never transmits or freezes a complete tree or every descendant level.

The original libstyx handles Twalk/Topen/Tstat/Tread/Tclunk. The C backend starts a Python helper without a shell; it requests HTTP(S), validates the entire JSON body and only then writes rows to a private pipe. That pipe is a local example detail, not a replacement for 9P or Unibus. The library acquires no HTTP or Python dependency.

The fixture computes a synthetic hierarchy with billions of possible objects without storing the tree. It is not real geographic information. An internet service or database can replace it using the schema below.

## Build and run

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON
cmake --build build -j
python3 examples/http_source.py 8080
```

From the package root; leave the fixture in the first terminal. Start the server in a second terminal and the client in a third:

```sh
./build/examples/styx-http 5640 http://127.0.0.1:8080/node
./build/examples/styx-cat 5640 Exampleland/city-2/district-3/street-4/house-5/room-6/info
```

Expect `Synthetic information for room-6` without LF. Choose available ports; each server announces the actual port in its first stdout line. Stop them with Ctrl-C; each helper exits after its request. The hosted example requires Linux64/libc/pthread and Python3 stdlib, without GUI or VM.

## Parameters and limits

| Parameter | Value and effect |
| --- | --- |
| styx-http argv1 | Decimal port 0..65535; default 0 selects an available port |
| styx-http argv2 | Required operator-supplied HTTP(S) endpoint |
| Endpoint query | Other parameters are preserved; op/id/start/limit and fragments are rejected |
| Helper args | endpoint, stat/list, unsigned64 id, unsigned64 start, limit 1..16 |
| HTTP timeout | 3s per operation; a 5s process alarm bounds the entire helper |
| Response body | At most 128KiB; UTF-8 JSON array |
| Page size | Up to 16 immediate children; stat returns at most one row |
| name / value | Up to 127 / 2047 UTF-8 bytes without NUL; value defaults to empty |

## HTTP schema

```text
GET /node?op=stat&id=0&start=0&limit=1
GET /node?op=list&id=0&start=0&limit=16
```

```json
[{"id": 9007199254740993, "parent": 0, "mode": 292, "name": "large", "value": "text"}]
```

JSON id/parent numbers are exact integers in 0..2⁶⁴−1: not floats, strings or booleans. Python preserves them exactly; a JavaScript backend must avoid rounding through Number. mode combines directory bit 0x80000000 and rwx bits 0777; 292 above means 0444.

Each row is an object with required id/parent/mode/name; optional value must be a string. Unknown extra fields are ignored. Duplicate JSON keys, NaN/Infinity, invalid UTF-8, NUL and exceeded limits fail. A declared Content-Length must be unique, bounded and fully received; chunked+Content-Length is rejected as ambiguous. A truncated HTTP body, even containing a complete [], never means absence. The root has id=0, parent=0, name="/" and the directory bit. Other names are nonempty, contain no / and are neither . nor ..; a nonzero ID cannot be its own parent.

For stat, the returned row ID must equal the requested ID; an empty array means absence. For list, every parent must equal the requested ID, and the directory cannot be its own child. Names and IDs are unique within each page. Validation never fetches other pages to check global duplicates: the source must preserve consistent IDs and ordering across pages.

## Changes and opened objects

start is a row index, not a 9P byte offset. C callbacks convert directory offsets to encoded Dir records. Twalk resolves components and Topen checks mode; there is no extra Twalk before every read/write. Later stat/read requests fetch metadata by the retained ID. Renaming that ID does not change the object of an opened fid; content is fetched again and may change.

This HTTP backend does not retain the remote record through a lease/refcount: if the source deletes an ID, the next stat/read fails. The source must never reassign that ID to another object. Listings changed between pages may require rewind offset0; this example promises no snapshot consistency. Large data needs a byte-stream read implementation instead of bounded text value.

## Errors and blocking

The helper validates the entire response before stdout: an invalid final row never publishes a valid prefix. An invalid nonempty response never becomes []/EOF. Nonzero exit, timeout or transport failure gives a backend error; the 9P callback returns Rerror and a healthy connection remains available for subsequent requests. C catalog_query may contain provisional output on failure: callers must check success and never consume it.

The projection is readonly: catalog_write always refuses and performs no HTTP mutation/retry. The HTTP backend is synchronous: its server owner waits for the helper. This is a teaching example, not an asynchronous multi-client HTTP scheduler. A production slow backend needs a separate owner/worker and deferred-read API; the codec requires no VM stop or global execution lock. Access policy and stable IDs remain the source’s responsibility.

## Checks and sources

```sh
ctest --test-dir build -R '^styx-http' --output-on-failure
```

The base styx-http test walks a deep lazy hierarchy with 16-row pages. styx-http-contract checks schema, duplicates, foreign IDs/parents, UTF-8 byte limits, absence of partial stdout and real 9P reads of IDs 2⁵³+1 and 2⁶⁴−1. A mutable fixture changes names and content between requests; ordinary reads on the same connection must recover after Rerror.

[HTTP validation/helper](../http_query.py) · [C backend](../http_catalog.c) · [Synthetic source](../http_source.py) · [Server callbacks](../server.c) · [Backend contract](../catalog.h) · [Contract tests](../../tests/http_contract.py)

[styxinit](../../docs/reference/styxinit.md) · [styxprocess](../../docs/reference/styxprocess.md) · [Main-project Styx help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
