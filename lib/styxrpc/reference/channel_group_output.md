# styxrpc_channel_group_output

[Русский](channel_group_output.ru.md) · [Declaration](../include/styxrpc/channel_group.h) · [Implementation](../src/channel_group.c) · [Operation fids](../src/channel_group_wire.c) · [Lease guide](channel_leases.md)

Include `<styxrpc/channel_group.h>` and link the hosted C99 target `StyxRpc::serve`.

```c
StyxRpcStatus styxrpc_channel_group_output(const StyxRpcChannelQueue *queues,
    size_t count, size_t root, StyxRpcChannelOutput *output);
```

| Parameter | Contract |
| --- | --- |
| `queues` | Borrowed queue descriptions. Use distinct positive 31-bit keys and exact element shapes. A queue capacity is 0–8 slots. Zero selects rendezvous. For occupied slots, supply their encoded byte lengths, buffers, and a stride of 1–65536 bytes. Supply `count` and `head` in each queue using the usual FIFO order. Empty queues can omit the input arena. |
| `count` | Number of queues, 1–8. |
| `root` | Zero-based index of the published result channel. Its shape must match the compiled result offer. |
| `output` | Caller-owned result storage. On success, its resource owns the copied group and is released using its `release` callback. |

Returns `STYXRPC_OK` with a transferable resource. Malformed types or frames, unknown or separately routed keys, conflicting capacity markers, and allocation failure produce `STYXRPC_INVALID` and preserve `output`. The builder copies the descriptions and queued messages. Source buffers are reusable immediately after return. Frame sizes must fit the native operation and reply limits. In a nested message, the group authorizes only its exact current-bus keys and canonicalizes the capacity marker for each received key. This preserves the same rendezvous identity when Limbo sends it back as a regular key.

Call this function while preparing a [channel result](channel_module_init.md) or in an [owner-driven handler](channel_handlers_create_with_results.md). The caller may also drive the resource directly from one serialized C owner. Open, read, write and close run on that owner. Complete or cancel all fids before release. With a module registry the issuing function fid owns the lease. The registry revokes its operation fids before running release. Unpublished resources can be released by the application owner.

The [standalone C check](../tests/channel_group.c) builds a FIFO containing a rendezvous, reuses the input arena, reads the nested key, and echoes it with its original capacity. After building the hosted SDK, run `ctest --test-dir build -R '^styxrpc-channel-group$' --output-on-failure`.

For a composite result, `root` selects the first live channel in result order. The later keys use the same resource and token. [Output contract](channel_module_init.md#composite-replies).
