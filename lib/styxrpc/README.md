# libstyxrpc

Copyright (c) 2026 bhgv

[Русский](README.ru.md)

Portable mounted RPC over Styx and UniBus S-records. A device registers C
functions and publishes a virtual module. DisVM loads it through ordinary
`load`. A C caller can encode requests and decode replies using its own
Styx connection.

[Mixed module images, typed imports and owner control files](MODULES.md).

## Protocol compatibility with DisVM

Native DisVM Styx RPC and the standalone C SDK share one wire contract.
For a given profile they agree on service paths, signatures, request and
reply framing, offsets, error handling, cancellation and fid lifetimes.
Protocol changes are coordinated across both implementations. A new
capability is advertised through its agreed profile once both ends can
handle it. The compatibility tests pair the independently built C SDK
with real DisVM and exercise the supported caller and provider roles.
[DisVM integration tests](examples/README.md#disvm-integration-tests)

[Runnable typed values example](examples/disvm/TYPED_VALUES.md). FlashVM sends full-width signed `big` values using the same SDK records as DisVM.

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

[Captured module directories](reference/module_open.md) keep discovered exports and calls bound to the selected image. See [export discovery](reference/module_exports_at.md) and [the exact scalar call](reference/scalar_invoke_at.md).
[Scoped discovery](reference/module_exports_scoped_at.md) reports which exact methods need a caller PID. During a synchronous call, the owner can [serve the native program tree](reference/server_program_step_for.md) while its application callbacks wait.
[Cancellable live loading](reference/module_live_open_until.md) releases the exact request fid when the caller stops waiting for a Dis image.
[Cancellable module capture](reference/module_capture_until.md), [import verification](reference/module_check_at_until.md) and [export discovery](reference/module_exports_scoped_at_until.md) keep checks relative to the exact mounted directory.
[Cancellable method calls](reference/scalar_invoke_at_until.md) distinguish a stop before the first write from an uncertain outcome after submission. The same contract applies to the managed, graph and channel codecs.
[Cancellable channel operations](reference/channel_lease_exchange_until.md) close the temporary operation fid while retaining the original route.
[Named channel enrollment](reference/named_open_until.md) can flush setup on a multiplexed session and retires the fid on cancellation.

[Custom UniBus messages](reference/custom_messages.md) let applications register their own types and handlers on an existing Styx session.

[Callback files](reference/module_slot_new_node.md) expose application nodes beside the hosted playback controls.

[Replaceable module images](reference/module_slot.md) share a server tree
with stable playback controls.

[Credited terminal mailbox](reference/unibus_terminal.md)

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

Use [managed invoke](reference/managed_invoke.md) to call the compiled managed profile from an independent program and [managed module publication](reference/managed_module_init.md) to export synchronous callbacks. The portable codec also exposes [integer decoding](reference/value_get_integer.md).

[Encode real](reference/value_real.md) and [decode real](reference/value_get_real.md) preserve binary64 bits in the existing Dis format.

## Process registration

Register the process and its variable, FD and VFS resources on the dispatcher, using fixed caller storage. Each open gets an independent cookie if the application supplies one.

[`styxrpc_programs_init`](reference/programs_init.md) · [`styxrpc_program_add`](reference/program_add.md) · [`styxrpc_program_fd`](reference/program_fd.md) · [`styxrpc_program_int`](reference/program_int.md) · [`styxrpc_program_variable`](reference/program_variable.md) · [`styxrpc_program_file`](reference/program_file.md) · [`styxrpc_program_unibus`](reference/program_unibus.md)

[`styxrpc_program_collection`](reference/program_collection.md) · [`styxintarrayopen`](reference/intarrayopen.md) · [`styxintarrayclose`](reference/intarrayclose.md) · [`styxintarrayselect`](reference/intarrayselect.md) · [`styxintarrayread`](reference/intarrayread.md) · [`styxintarraywrite`](reference/intarraywrite.md)

## Limbo source

Generate a .m from the same formal types used for native dispatch. Supply ADT field names and use Limbo for the compiled signature.

[`styxrpc_module_source`](reference/module_source.md)

## Nested reference graphs

Direct mounted Dis calls use the positional value graph protocol for nested and returned refs. A standalone C program can invoke a graph or publish a module through the hosted SDK. Both use the same compiled interface, positional frames and sparse updates as DisVM. The independent C client example checks a nested update over a Styx fid.

[`styxrpc_graph_profile`](reference/graph_profile.md) · [`styxrpc_graph_request`](reference/graph_request.md) · [`styxrpc_graph_arguments`](reference/graph_arguments.md) · [`styxrpc_graph_reply`](reference/graph_reply.md) · [`styxrpc_graph_read`](reference/graph_read.md)

[`styxrpc_graph_invoke`](reference/graph_invoke.md) · [`styxrpc_graph_module_init`](reference/graph_module_init.md) · [Sparse graph updates](reference/graph_updates.md)

## Hybrid tables in RPC graphs

The native Dis shape `h` carries one table identity with numeric and counted text keys. The C SDK supplies the same storage, ordered snapshot codec and key-level patches for standalone callers and providers. [C guide and API](reference/hybrid_tables.md).

## UniBus channel frames

Dis passes live channels to a native C export. C operates on the reverse `.prog/<pid>/.unibus` and can receive a nested channel in the same call. A returned C-owned channel remains leased through the function fid. The portable C scheduler handles FIFO and synchronous channels, guard selection and clunk cancellation.

[`styxrpc_channel_request`](reference/channel_request.md) · [`styxrpc_channel_read`](reference/channel_read.md) · [`styxrpc_channel_reply`](reference/channel_reply.md) · [`styxrpc_channel_result`](reference/channel_result.md) · [`styxrpc_channel_fid_write`](reference/channel_fid_write.md) · [`styxrpc_channel_fid_finish`](reference/channel_fid_finish.md) · [`styxrpc_channel_fid_read`](reference/channel_fid_read.md) · [`styxrpc_channel_fid_close`](reference/channel_fid_close.md)

[`styxrpc_channel_profile`](reference/channel_profile.md) · [`styxrpc_channel_call`](reference/channel_call.md) · [`styxrpc_channel_call_reply`](reference/channel_call_reply.md) · [`styxrpc_channel_export_value`](reference/channel_export_value.md) · [`styxrpc_channel_exchange`](reference/channel_exchange.md)

[`styxrpc_channel_import_element`](reference/channel_import_element.md) · [`styxrpc_scheduler_init`](reference/scheduler_init.md) · [`styxrpc_scheduler_admit`](reference/scheduler_admit.md) · [`styxrpc_scheduler_cancel`](reference/scheduler_cancel.md) · [`styxrpc_scheduler_progress`](reference/scheduler_progress.md)

## Named launch and dynamic tree

A persistent DisVM provider can launch several named images on one Styx connection. Each launch keeps its own read/write fid. The C peer publishes its reverse `.prog/<pid>` context using `StyxRpc::serve`. The server calls the application to enumerate and resolve directories on demand. A changed revision ends a stale directory scan. Already open file cookies retain their own state.

[`styxrpc_launch_named`](reference/launch_named.md) · [`styxrpc_launch_named_status`](reference/launch_named_status.md) · [`styxrpc_launch_named_close`](reference/launch_named_close.md) · [`styxrpc_server_create`](reference/server_create.md) · [`styxrpc_server_enable_mutations`](reference/server_enable_mutations.md) · [`styxrpc_server_step`](reference/server_step.md) · [`styxrpc_server_destroy`](reference/server_destroy.md) · [`styxrpc_server_state`](reference/server_state.md) · [`styxintvariableread`](reference/intvariableread.md) · [`styxintvariablewrite`](reference/intvariablewrite.md)

The hosted client also supports [stat](reference/vfs_stat.md), [positional read](reference/vfs_read_at.md) and [positional write](reference/vfs_write_at.md). These operations use an already mounted session, hold one fid, and report the confirmed transfer prefix.

[Cancellable reads](reference/vfs_file_read_until.md) and
[writes](reference/vfs_file_write_until.md) keep the confirmed
prefix if a later chunk is stopped. The paired DisVM server handles
both as deferred transfers using standard Styx `Tflush`.

[Deferred file transfers](DEFERRED_IO.md) let ordinary C providers select read and
write callbacks that can run on native workers.

[VFS control cancellation](VFS_CONTROL.md) covers path lookup, open, directory
iteration and namespace mutations.

Use [open](reference/vfs_open.md) for a persistent descriptor with [stat](reference/vfs_file_stat.md), [read](reference/vfs_file_read.md), [write](reference/vfs_file_write.md) and [close](reference/vfs_close.md). [List](reference/vfs_list.md) reads a mounted directory. Native external int variables use [one-value reads](reference/vfs_int_read.md) and [writes](reference/vfs_int_write.md). Use [create](reference/vfs_create.md) for an exclusive write-only create.
Use [mkdir](reference/vfs_mkdir.md) to create a directory and [remove](reference/vfs_remove.md) to remove an ordinary entry.

[Named rendezvous channels](reference/named_channel.md) use an attached session
and let providers claim and release exact names in the VFS.
The [hosted peer service](reference/named_peer_host.md) serves one registry
for two native clients, including a local client in the provider process.
Use [executable callback nodes](reference/node_exec.md) for a synchronous
byte request and reply on a path published by the peer.
[Shared value cells](reference/named_cell.md) travel by reference.
[Runnable C and Limbo example](examples/shared-cell/README.md) demonstrates cell cancellation and lease lifetime.
[Paired module references](reference/module_refs.md) carry the identity
and access mode of a live mounted Int32 export through that service.

The Int32 API supports cancellable [identified open](reference/vfs_int_open_identified_until.md),
[read](reference/vfs_int_read_until.md) and [write](reference/vfs_int_write_until.md).
Use [cancellable PID resolution](reference/int_ref_open_until.md) for process grants.
[Runnable FlashVM and DisVM example](examples/disvm/INT32_REFERENCES.md).
Embeddings can use a cancellable reference port for [reads](reference/module_ref_port_read_until.md)
and [writes](reference/module_ref_port_write_until.md).
[Runnable Limbo and FlashVM example](examples/disvm/REF_PORT_CANCELLATION.md).

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

## Reverse channel arguments

[Channel argument scopes](reference/channel_argument_scopes.md) lend owner-local queues on a stable reverse process bus. The caller verifies its exact scoped offer and keys, publishes only for that invocation, and revokes pending operations on return.

[Hosted channel handlers](reference/channel_handler_modules.md) accept Dis arguments in an independent C application. The Styx dispatcher acknowledges requests and serves replies while the application callback uses the reverse bus on its own thread. The hosted target requires POSIX threads.

## Leased channel clients and modules

[Hosted channel leases](reference/channel_leases.md) provide verified scoped invocations, pinned result-channel operations and callback-backed channel publication. C programs and the FlashVM bridge share these adapters.

An import can now stop while confirming a [module token](reference/channel_lease_import_message_until.md) or [reverse PID](reference/channel_pid_import_until.md). All routes from a message are published together.
[Runnable C and DisVM example](examples/disvm/CHANNEL_ADMISSION.md).

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

When the application owner and the reverse dispatcher run on separate threads, use a [program server](reference/program_server_create.md). The host can [publish](reference/program_server_grant.md) and [revoke](reference/program_server_revoke.md) a PID while the dispatcher runs. A [borrowed queue resource](reference/channel_group_reverse_borrow.md) lets the peer use an existing by-value FIFO or rendezvous and returns the advanced cursor to the owner after revocation.

[Borrow a queue graph](reference/channel_group_reverse_borrow_graph.md) to expose several existing FIFOs and rendezvous under one PID. Nested channel leaves use keys on the same bus. Revocation returns the live frames and cursors to the original descriptors.

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
