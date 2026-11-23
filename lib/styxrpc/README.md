# libstyxrpc

Copyright (c) 2026 bhgv

[Русский](README.ru.md)

Portable mounted RPC over Styx and UniBus S-records. A device registers C
functions and publishes a virtual module. DisVM loads it through ordinary
`load`. A C caller can encode requests and decode replies using its own
Styx connection.

## Integration scenario

An external application, such as a FlashVM host or a device controller,
associates export names and types with its C callbacks and publishes a
module in a Styx tree. DisVM mounts that tree in its VFS. Ordinary Limbo
`load` checks `.interface`, then each call uses its own function fid.
The portable library checks UniBus S-records using application buffers.
The hosted adapters manage the Styx connection and open resources.

In the other direction, a C client can call DisVM functions or launch
named images. Both peers can serve requests over one bidirectional
connection. The reverse `.prog/<pid>` tree exposes variables,
descriptors, VFS and a UniBus endpoint. Results can include references
and live channels. Ordinary Limbo operations, including `alt`, use them
within the granted lifetime. Closing the fid retires associated state
and cancels pending operations.

[End-to-end examples and acceptance](examples/README.md#disvm-integration-tests)

For hosted scalar modules, publish descriptors with [styxrpc_scalar_module_init](reference/scalar_module_init.md) and call verified exports with [styxrpc_scalar_invoke](reference/scalar_invoke.md). The standalone C client and the FlashVM bridge use the same client API. FlashVM links to the installed SDK. The SDK has no dependency on either VM.

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

## Process registration

Register the process and its variable, FD and VFS resources on the dispatcher, using fixed caller storage. Each open gets an independent cookie if the application supplies one.

[`styxrpc_programs_init`](reference/programs_init.md) · [`styxrpc_program_add`](reference/program_add.md) · [`styxrpc_program_fd`](reference/program_fd.md) · [`styxrpc_program_int`](reference/program_int.md) · [`styxrpc_program_variable`](reference/program_variable.md) · [`styxrpc_program_file`](reference/program_file.md) · [`styxrpc_program_unibus`](reference/program_unibus.md)

[`styxrpc_program_collection`](reference/program_collection.md) · [`styxintarrayopen`](reference/intarrayopen.md) · [`styxintarrayclose`](reference/intarrayclose.md) · [`styxintarrayselect`](reference/intarrayselect.md) · [`styxintarrayread`](reference/intarrayread.md) · [`styxintarraywrite`](reference/intarraywrite.md)

## Limbo source

Generate a .m from the same formal types used for native dispatch. Supply ADT field names and use Limbo for the compiled signature.

[`styxrpc_module_source`](reference/module_source.md)

## Nested reference graphs

Direct mounted Dis calls can use the positional value-only graph protocol. The example updates a nested object and preserves its Limbo identity.

[`styxrpc_graph_profile`](reference/graph_profile.md) · [`styxrpc_graph_request`](reference/graph_request.md) · [`styxrpc_graph_arguments`](reference/graph_arguments.md) · [`styxrpc_graph_reply`](reference/graph_reply.md) · [`styxrpc_graph_read`](reference/graph_read.md)

## UniBus channel frames

Dis passes live channels to a native C export. C operates on the reverse `.prog/<pid>/.unibus` and can receive a nested channel in the same call. A returned C-owned channel remains leased through the function fid. The portable C scheduler handles FIFO and synchronous channels, guard selection and clunk cancellation.

[`styxrpc_channel_request`](reference/channel_request.md) · [`styxrpc_channel_read`](reference/channel_read.md) · [`styxrpc_channel_reply`](reference/channel_reply.md) · [`styxrpc_channel_result`](reference/channel_result.md) · [`styxrpc_channel_fid_write`](reference/channel_fid_write.md) · [`styxrpc_channel_fid_finish`](reference/channel_fid_finish.md) · [`styxrpc_channel_fid_read`](reference/channel_fid_read.md) · [`styxrpc_channel_fid_close`](reference/channel_fid_close.md)

[`styxrpc_channel_profile`](reference/channel_profile.md) · [`styxrpc_channel_call`](reference/channel_call.md) · [`styxrpc_channel_call_reply`](reference/channel_call_reply.md) · [`styxrpc_channel_export_value`](reference/channel_export_value.md) · [`styxrpc_channel_exchange`](reference/channel_exchange.md)

[`styxrpc_channel_import_element`](reference/channel_import_element.md) · [`styxrpc_scheduler_init`](reference/scheduler_init.md) · [`styxrpc_scheduler_admit`](reference/scheduler_admit.md) · [`styxrpc_scheduler_cancel`](reference/scheduler_cancel.md) · [`styxrpc_scheduler_progress`](reference/scheduler_progress.md)

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

## Reverse `table of big`

The application owns the positional and keyed parts of the table. One C callback handles operations on both parts. `.idx` distinguishes numeric and string keys, `.len` reports the dense prefix, and `.count` includes the keyed part. This complements the Int32 collection API.

[`styxrpc_program_bigtable`](reference/program_bigtable.md) · [Working example](examples/README.md)

## Delegated channels

A nested channel can use a key on the current bus, a distinct reverse PID or
a module token. The application binds the grant to the original Styx
connection. The publisher owns the resource lifetime. On C endpoints a
callback authorizes nested addresses in typed messages, and the adapter
opens a separate fid on the exact `.unibus`. A published delegated PID must
be unique within the session. A module token remains valid while the issuing
fid holds its lease.

[`styxrpc_channel_element_routes`](reference/channel_element_routes.md) · [`styxrpc_channel_export_routes`](reference/channel_export_routes.md) · [`styxrpc_channel_route_value`](reference/channel_route_value.md) · [`styxrpc_channel_read_routed`](reference/channel_read_routed.md) · [`styxrpc_channel_route_exchange`](reference/channel_route_exchange.md)

The C dispatcher can revoke a reverse bus immediately. Pending operations retire before the call returns, and old fids can only be clunked. [`styxrpc_program_unibus_revoke`](reference/program_unibus_revoke.md) The registry issues fresh PIDs for temporary buses and reuses their slots after revocation. [`styxrpc_program_unibus_grant`](reference/program_unibus_grant.md)


## Shared C module bus

The hosted library lets several opened calls return independent channels on
one `.unibus` file. The token selects a queue, and each operation fid holds
its own open state. The registry handles tokens and revocation. The
application owns the queues, supplies the registry storage and ties
revocation to the issuing fid. Use `StyxRpc::serve` on the publisher and
`StyxRpc::launch` for a C caller operating on a received route.

[`styxrpc_module_channels_init`](reference/module_channels_init.md) · [`styxrpc_module_channels_publish`](reference/module_channels_publish.md) · [`styxrpc_module_channels_resource`](reference/module_channels_resource.md) · [`styxrpc_module_channels_revoke`](reference/module_channels_revoke.md)

[Two channels over one socket](examples/module_channels.md)

## Sources

[Calls and profiles](include/styxrpc/rpc.h) · [Virtual files](include/styxrpc/vfs.h) ·
[Format provenance](SOURCES.jsonl)
