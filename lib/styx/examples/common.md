# Shared server: request to backend and back

Copyright (c) 2026 bhgv

[English](common.md) · [Русский](common.ru.md) · [Examples](README.md)

## Purpose

Five catalog models share one C server: fixed (flat/tree), database, live and HTTP. Only the backend behind the private catalog.h contract changes. These are runnable teaching adapters, not separate protocol implementations: the original libstyx handles wire messages, fids and their retirement.

The libraries and shared server are implemented in C. Python wire_client/check are independent test clients; the HTTP helper and fixture belong to the HTTP example. They do not turn the C codec, libstyx or libunibus into Python/C++ libraries.

## Source map

| File | Responsibility |
| --- | --- |
| [server.c](server.c) | Original Styxops callbacks, byte offsets, metadata packing |
| [catalog.h](catalog.h) | Private backend inputs, result validity, open/close contract |
| [fixed.c](fixed.c) | Immutable flat/tree selection through CATALOG_TREE |
| [database.c](database.c) | Fresh bounded TSV query, logical key identities |
| [live.c](live.c) | Published names versus retained opened objects |
| [http_catalog.c](http_catalog.c) | Owned helper process and private pipe |
| [client.c](client.c) | Native retained-fid reader, styx-cat |
| [wire_client.py](wire_client.py) | Independent standard 9P test client |
| [check.py](check.py) | Process ownership and five model acceptance cases |


## Request flow

1. `main` in [server.c](server.c) calls catalog_init before styxinit. Fixed/live need no source; database/HTTP borrow the argv string until exit. The backend does not construct a remote tree at startup.
2. Twalk invokes [walk_node](server.c#L58) for a component: validate the current directory ID, find an immediate child through pages, or resolve `..`. Qid changes only after success. No extra Twalk precedes every read/write.
3. Tstat invokes [stat_node](server.c#L108); [attributes](server.c#L36) builds Dir with borrowed strings. The dispatcher packs them before the next callback. Metadata storage is shared, so callbacks run under one server owner.
4. Topen invokes [open_node](server.c#L128): check current mode, permit exact OREAD/OWRITE and acquire one pin on success. There is no implicit create/truncate. Root/metadata grant no extra credentials; mode is demonstration policy here.
5. Tread invokes [read_node](server.c#L169) using the retained Qid and byte offset. Files yield current text; directories yield whole Dir records for immediate children. A whole namespace is never serialized.
6. Twrite invokes [write_node](server.c#L235) once and preserves the accepted byte count. Only live accepts ctl commands; the other backends refuse writes. A short write is never promoted to full acknowledgement or replayed.
7. Tclunk or disconnect invokes [close_node](server.c#L149) for a successful open. Unpin uses the original ID without a name lookup/reopen. SIGINT/SIGTERM makes main leave its loop, then styxend retires server resources.

## Pages, offsets and errors

catalog_query takes start and capacity in rows. At the 9P layer, a directory offset counts bytes of encoded Dir records. read_node scans pages and accumulates record sizes until it reaches the offset. A record is never split across replies; an undersized first buffer fails, and an offset inside a record reports rewind. Changes between requests can require offset0; no snapshot is promised.

query success=1 with count=0 means absence/end. success=0 means failure; output/count are provisional and unused. Stat/walk metadata failure maps to Enonexist, listing failure to backend query failed: both are Rerror, not a successful empty read. Buffers partly filled before failure are not confirmed data.

## Backend lifetimes

Fixed rows live for the entire process: pin/unpin acquire no resources. Live retains a shared directory/title/button slot; hiding a name does not delete an opened object, final unpin frees the slot, and IDs are never reused. Database/HTTP acquire no record lease: an opened fid retains a logical ID, but later reads query the source again and may fail after deletion. A name and its underlying logical identity are distinct.

TSV is bounded to128 keys; FNV64 is a demonstration key ID and collisions/duplicates in the current set fail. Do not treat the hash as an access token. The HTTP helper uses a3s operation timeout,5s alarm and128KiB body limit. These backends are synchronous: the server owner waits for I/O. This is not a production asynchronous scheduler; slow sources need a separate owner/worker and deferred API. No global VM/execution-thread stop is required.

## Running the native client

```sh
./build/examples/styx-flat 5640
```

In another terminal:

```sh
./build/examples/styx-cat 5640 status
```

[client.c](client.c) takes exactly PORT PATH, connects to loopback, transfers its owned descriptor to styxclientmountfd and then owns client/fid. The constructor consumes the descriptor on failure too. The client walks/opens, reads confirmed chunks and advances only by actual count. Confirmed zero is EOF; error/uncertain means failure, never replay. It then clunks, shuts down and frees. Exit codes:0 EOF,1 arguments/socket,2 mount,3 walk/open/read/short fwrite. This is a blocking text utility, not a reconnecting/general shell client.

[styxinit](../docs/reference/styxinit.md) · [styxprocess](../docs/reference/styxprocess.md) · [styxend](../docs/reference/styxend.md) · [styxclientmountfd](../docs/reference/styxclientmountfd.md) · [styxclientread](../docs/reference/styxclientread.md) · [convD2M](../docs/reference/convD2M.md)
