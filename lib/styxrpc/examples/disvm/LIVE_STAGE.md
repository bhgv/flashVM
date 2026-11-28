# Use live VFS fields, copied collections and scoped calls in one bridge

[Русский](LIVE_STAGE.ru.md) · [SDK examples](../README.md) · [C live APIs](../../LIVE_VALUES.md) · [FlashVM types](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_VALUE_TYPES.md)

The example pairs a native Limbo owner and an AS2 movie. One library provides a by-value list and a sparse-array method, then mutates a ref or a hybrid table in a call with a result channel. A separate mounted record gives the movie live access to the owner's variables. The independent C example calls the same library and accesses the same VFS projections with its own hosted session.

## Build and run

From the repository root, build the native DisVM and Limbo bootstrap, FlashVM's hosted Styx player and the AS2 compiler pipeline. Use the paths below or substitute paths to your built tools. The SDK example additionally needs a hosted libstyxrpc configured with `STYXRPC_EXAMPLES=ON` and built with the installed `Styx` and `Unibus` packages. Build its `styxrpc-live-stage-client` target. [FlashVM build](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC.md#build) · [SDK build](../../README.md#build).

```sh
python3 lib/styxrpc/tests/flashvm/mixed_pair.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --player build/flash/flashvm_styx_player \
    --as2 out/as2/as2_viewc --fir out/fir/fir1 \
    --fir-opt out/fir/fir1_opt --swl out/linker/swl --live-stage
```

This command compiles the Limbo library and movie, opens a paired mount with live load enabled, and starts the owners. The default is a Unix socket and the interpreter. Add `--jit` for native JIT and `--tcp` for loopback TCP. Both options can be combined. Success prints `PASS native live-stage RPC paired VMs` and `PASS Dis application completed`. An incorrect value or missing rendezvous fails the launcher. Temporary images and connections are cleaned up on exit.

## Follow the VFS publication

1. The [Limbo owner](../../tests/flashvm/live-stage_pair.b) publishes a record at `/live/.var`. The `numbers` field is an Int32 array whose positions 0 and 2 share one native cell. `details` contains a string cell, a vector and a scalar `big`. `scalar` names the original shared Int32.
2. The [AS variable exercise](../../tests/flashvm/live-stage_values.as) captures the record and fields. It writes `42` at index 0 and reads `42` from both index 2 and `scalar`. It sets a counted Unicode string and round-trips both signed boundaries across the wide fields. The owner independently reads the same Int32 with `Vfsexport.intget`. [Projection table](../../LIVE_VALUES.md#projection-contract-for-providers)
3. The [worker](../../tests/flashvm/live-stage_stale.as) captures a writable field before the owner unmounts the record. The owner installs a new record at the same path and signals the worker. When it tries to write through the old field the operation raises a stale exception. The [movie](../../tests/flashvm/live-stage_pair.as) opens the new generation and reads `99`. The owner confirms the old cell stayed at `42`. Each normal field and root is explicitly closed, and the worker retires its remaining handles when it ends.

## What ActionScript calls

At startup, `file.load()` returns `proxy` for the compiled `Library` module. This table lists every method the example calls through that proxy. [AS2](../../tests/flashvm/live-stage_pair.as#L12)

| AS2 call | Limbo declaration | Implementation |
| --- | --- | --- |
| `proxy.words(list)` | [`Library.words`](../../tests/flashvm/live-stage_library.b#L11) | [`words`](../../tests/flashvm/live-stage_library.b#L21) |
| `proxy.sparse(packed)` | [`Library.sparse`](../../tests/flashvm/live-stage_library.b#L12) | [`sparse`](../../tests/flashvm/live-stage_library.b#L41) |
| `proxy.combine(refFields, input)` | [`Library.combine`](../../tests/flashvm/live-stage_library.b#L13) | [`combine`](../../tests/flashvm/live-stage_library.b#L59) |
| `proxy.newtable()` | [`Library.newtable`](../../tests/flashvm/live-stage_library.b#L14) | [`newtable`](../../tests/flashvm/live-stage_library.b#L74) |
| `proxy.update(table)` | [`Library.update`](../../tests/flashvm/live-stage_library.b#L15) | [`update`](../../tests/flashvm/live-stage_library.b#L83) |

The hybrid table type is declared in the Bigtable module. [`Bigtable->Handle`](../../../../disVM/module/bigtable.m#L10)

## Send a list or sparse array as a copy

In the [compiled Limbo library](../../tests/flashvm/live-stage_library.b), `words` declares `list of (string, big)`. For `l[(sb)]` the movie sends a dense Array of two-field tuples, or `null` for nil. A zero-length input Array also maps to nil. Limbo uses `hd` and `tl`, changes the order and adjusts the wide integer fields. The [AS call exercise](../../tests/flashvm/live-stage_calls.as) observes the new list as a local dense Array. It separately checks that a nil result becomes `null`.

`sparse` uses the explicit Limbo tuple `(int, array of (int, string))`. Its logical length is independent of the number of present indices. Pack the Array, call the method, then unpack the returned tuple.

```actionscript
var values = [];
values[5] = "end";
values[2] = "☀";
values.length = 12;                   // Include holes after the last own slot.
var packed = vfs.packSparse(values);  // [12, [[2, "☀"], [5, "end"]]]
var edited = proxy.sparse(packed);    // Limbo moves the second slot to index 9.
var restored = vfs.unpackSparse(edited);
var hasOldSlot = restored.hasOwnProperty("5"); // false. The original still has index 5.
```

The result has length 12 and values at indices 2 and 9. Check ownership with `hasOwnProperty` to distinguish an absent index from any value at that index. The typed element here is `string`. Its value must conform to the compiled schema. A plain Limbo `array of string` uses the dense mapping. [Typed managed calls](../../reference/managed_invoke.md) · [Sparse API](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_PACK_SPARSE.md).

## Apply copy-back and acquire a channel together

`combine` takes a one-field `ref Box` plus a lent `chan of int`. It receives once, updates the ref, then returns a tuple with that ref and a new `chan of big`. The compiled argument and result lists are `(r[b]c[i])` and `((r[b]c[b]))`. The [movie exercise](../../tests/flashvm/live-stage_mixed.as) uses the same AS Array for the returned ref and the original argument. It proves the alias by assigning through the returned ref. Use the queue after releasing the module proxy. Its independent lease holds the issuing generation until close.

Here are the Limbo type and function declarations. The field `box.value` maps to `refFields[0]` in AS2. [Limbo source](../../tests/flashvm/live-stage_library.b#L8-L13)

```limbo
Box: adt {
    value: big;
};
combine: fn(box: ref Box, input: chan of int): (ref Box, chan of big);
```

```actionscript
var refFields = ["-9223372036854775808"];
var input = vfs.localChannel("i", 1);
var sent = input.trySend(7);                   // Seed a buffered input for the native receive.
var result = proxy.combine(refFields, input);  // The owner returns the same ref and a new queue.
var copyback = refFields[0];                   // "-9223372036854775801"
var output = result[1];                        // One result channel with its own lease.
var inputClosed = input.close();
var unloaded = os.unload(proxy);
var observed = output.recv();                  // The leased value remains readable.
var outputClosed = output.close();
```

The movie also passes a hybrid table to `update`. The callback adds key `"wide"` and returns its own channel. The changed key is applied when the reply validates. The earlier table object remains the same. A scoped call keeps a single identity table for args, result and updates. A borrowed argument queue belongs to the caller and its grant ends with the call. A result queue is owned independently until released. [Leased callers](../../reference/channel_invoke.md) · [Graph updates](../../reference/graph_update_index.md).

## Run the C client against native DisVM

```sh
python3 lib/styxrpc/tests/flashvm/live_stage_sdk.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --driver build/rpc/styxrpc-live-stage-client
```

The same two mode flags apply. The [C driver](live_stage_client.c) has two roles.

| Role | Steps |
| --- | --- |
| Invoke typed methods | Publish a reverse caller PID, read compiled offers, send the `l[(sb)]` list and verify the reply and nil case. Invoke the table-plus-channel method, stage `result.updates` against the original table, verify the unrelated key and receive the exact signed minimum from the owned lease. Release the lease before closing the client. |
| Operate on live fields | Start a mounted client, open the [Limbo guest's record](../../tests/flashvm/live-stage_sdk.b), and capture four fields and release the parent fid. Distinguish nil from empty text, then write a five-byte string containing zero. Update the scalar `big` to its signed maximum. Select a relative dense index and update a sparse numeric key on a hybrid table. Write an Int32 element and read the matching alias. Submit the completion marker only after all readbacks succeed. Close the fields and then the session. |

Expect `PASS independent C SDK and native DisVM`, `PASS DisVM observed C SDK writes` and `PASS Dis application completed`. The SDK validates a leased reply and lends record views into the caller's buffer. The application stages and applies its own validated updates before reusing that buffer. The [hosted session APIs](../../reference/live_open_until.md) operate without either VM when the peer provides the same files.
