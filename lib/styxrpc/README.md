# libstyxrpc

Copyright (c) 2026 bhgv

[Русский](README.ru.md)

Portable mounted RPC over Styx and UniBus S-records. A device registers C
functions and publishes a virtual module. DisVM loads it through ordinary
`load`. A C caller can encode requests and decode replies using its own
Styx connection.

## Build

Install [libunibus](../libunibus/README.md) first. The core is C99 and links
`Unibus::wire`. It uses application-owned memory. The optional hosted device
example also needs [Styx](../styx/README.md) with `Styx::phy_server`.

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/prefix
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /path/to/prefix
```

Consumers can use `find_package(StyxRpc CONFIG REQUIRED)` and link
`StyxRpc::rpc`. Set `STYXRPC_EXAMPLES=ON` to build the hosted example.

## Interfaces

| Syntax | Parameters and description | Result |
| --- | --- | --- |
| [`styxrpc_profile`](reference/profile.md) `(exports, count, bytes, capacity, size)` | Export names, signatures, callbacks and contexts | DisVM-compatible `.interface` bytes |
| [`styxrpc_request`](reference/request.md) `(signature, argument, bytes, capacity, size)` and [`styxrpc_reply`](reference/reply.md) `(signature, bytes, size, value)` | Compiled signature and caller-owned buffer | Encoded request and decoded reply |
| [`styxrpc_open`](reference/open.md) `(call, exported)`, [`styxrpc_write`](reference/write.md) `(call, offset, bytes, size)`, [`styxrpc_read`](reference/read.md) `(call, offset, bytes, capacity, size)` | State for a single opened fid, contiguous request writes and positional reply reads | One callback after a complete request, followed by reply bytes |
| [`styxrpc_dir_entry`](reference/dir_entry.md) `(ops, context, directory, index, entry)` | Directory ID and child index | One entry per call. A callback's `NULL` ends enumeration and returns `STYXRPC_MORE` |
| [`styxrpc_file_open`](reference/file_open.md) / [`styxrpc_file_read`](reference/file_read.md) / [`styxrpc_file_write`](reference/file_write.md) / [`styxrpc_file_close`](reference/file_close.md) | Callback table, node ID, selected mode and state for a single open | Acknowledged byte counts and release of open state |

`STYXRPC_OK` indicates success. `STYXRPC_MORE` on a function fid indicates a
partial request or a pending reply. `STYXRPC_REMOTE_ERROR` indicates an
acknowledged provider error. `STYXRPC_INVALID` indicates invalid framing,
signature, mode or arguments. The current callback signatures are
`fn(int): int` and `fn(big): big`. Keep descriptors and callback tables
alive until their fids close. The dispatcher serializes each fid. The function
callback runs synchronously when a complete request arrives.

[Running the examples](examples/README.md)

## Managed calls and launch

The managed API accepts strings, arrays and tuples using the existing Dis shape grammar. For supported flat signatures, the callback selects changed ref arguments by bit mask. Return `STYXRPC_MORE` from begin to complete later, and route clunk or disconnect to close. Compile Limbo callers with `-D RG` to retain resolved import types.

Enable `STYXRPC_HOSTED` for `StyxRpc::launch`. The native client drives the current **one-shot** raw-profile launch service over Styx. Delivery and exit status are separate observations. The portable core continues to need only `Unibus::wire`.

[`styxrpc_managed_profile`](reference/managed_profile.md) · [`styxrpc_managed_open`](reference/managed_open.md) · [`styxrpc_managed_write`](reference/managed_write.md) · [`styxrpc_managed_finish`](reference/managed_finish.md) · [`styxrpc_managed_read`](reference/managed_read.md) · [`styxrpc_managed_close`](reference/managed_close.md) · [`styxrpc_managed_request`](reference/managed_request.md) · [`styxrpc_managed_reply`](reference/managed_reply.md) · [`styxrpc_value_integer`](reference/value_integer.md) · [`styxrpc_launch_start`](reference/launch_start.md) · [`styxrpc_launch_status`](reference/launch_status.md)

## Named launch and dynamic tree

A persistent DisVM provider can launch several named images on one Styx connection. Each launch keeps its own read/write fid. The C peer publishes its reverse `.prog/<pid>` context using `StyxRpc::serve`. The server calls the application to enumerate and resolve directories on demand. A changed revision ends a stale directory scan. Already open file cookies retain their own state.

[`styxrpc_launch_named`](reference/launch_named.md) · [`styxrpc_launch_named_status`](reference/launch_named_status.md) · [`styxrpc_launch_named_close`](reference/launch_named_close.md) · [`styxrpc_server_create`](reference/server_create.md) · [`styxrpc_server_step`](reference/server_step.md) · [`styxrpc_server_destroy`](reference/server_destroy.md) · [`styxrpc_server_state`](reference/server_state.md) · [`styxintvariableread`](reference/intvariableread.md) · [`styxintvariablewrite`](reference/intvariablewrite.md)

## Name, Qid and fid

`Twalk` resolves a child name to a stable node ID. The example uses
`Qid.path` for that ID. At `Topen`, the dispatcher passes the ID and access
mode to the callback and attaches the returned cookie to the opened fid.
`Tread` and `Twrite` receive its cookie, offset and size. A second open of the
same node has its own state and can operate on the same shared value.
`Tclunk` and disconnect release each opened state once.

The original device sample publishes a static tree. The reverse launch sample uses live lookup and directory enumeration.

## Example

The [C device](examples/device.c) publishes `/mounted/device.dis`. Its
descriptor generates `.interface`. The [Limbo client](examples/disvm/client.b)
loads the module, invokes `sample` twice, and reads and writes the virtual file
`value`. The [test launcher](examples/disvm/run.py) connects the processes
with a socket and mounts the device via `--styx-mount-fd FD --raw --at /device`.
Use `--jit` to run the same test in JIT mode.

Remaining work includes generated `.m` source, graph-aware nested refs, generic variable and FD registration, typed channels with `alt`, and richer grants.

## Sources

[Calls and profiles](include/styxrpc/rpc.h) · [Virtual files](include/styxrpc/vfs.h) ·
[Format provenance](SOURCES.jsonl)
