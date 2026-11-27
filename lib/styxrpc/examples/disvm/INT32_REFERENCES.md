# Cancel access to a live Int32 export

[Русский](INT32_REFERENCES.ru.md) · [Examples](../README.md) · [Module reference protocol](../../reference/module_refs.md)

The DisVM process publishes a writable Int32 and keeps its value on the
Limbo side. It sends a reference over the paired named-channel service.
An AS2 task opens the file on the issuing peer's ordinary VFS when it
first uses the reference. The fixture interrupts the task at five points
and then opens the same export in a new task.

## Build and run

Use a native Linux64 DisVM and a FlashVM player with StyxRPC enabled.
Build the AS2 frontend, FIR checker and optimizer, and SWF linker.
The [FlashVM player build guide](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_PLAYER.md)
and [DisVM build guide](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/STYX-CHANNEL.md)
cover the corresponding binaries. Run from the checkout that contains
both projects, using these conventional build paths or replacing them
with the paths to your binaries.

```sh
python3 lib/styxrpc/tests/flashvm/mixed_pair.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --player build/flash/flashvm_styx_player \
    --as2 out/as2/as2_viewc --fir out/fir/fir1 \
    --fir-opt out/fir/fir1_opt --swl out/linker/swl \
    --cancel-int-ref
```

`--source` supplies the DisVM modules and Limbo sources. The launcher
compiles those sources with `--limbo` and starts the resulting image
with `--runtime`. It compiles the AS2 task and movie to FIR, checks and
optimizes the FIR, emits objects and links a SWF before starting
`--player`. The default uses the interpreter and Unix socket pairs.
Use `--jit` for native compilation and `--tcp` for loopback streams.
The launcher owns the temporary directory, processes and socket ends.
Successful completion includes `PASS native cancel-int-ref RPC paired VMs`.

When configuring the hosted SDK's DisVM and FlashVM tests, also set
`STYXRPC_FLASHVM_TOOLS` to the tool build directory, `out` in the
command above. With `STYXRPC_FLASHVM_PLAYER` set, this registers all four
mode and transport combinations under `styxrpc-native-int-ref-*`.
Run them with `ctest --test-dir build/rpc -R styxrpc-native-int-ref --output-on-failure`
if the SDK build is at `build/rpc`.

## Follow the exchange

1. The Limbo owner publishes `/mounted/counter.dis/.var/value` using
   `Styxnodes->publishat` with mode `rw` and length four. Its service loop
   starts at `17` and accepts whole values at offset zero. `capturelocal`
   records that publication's Qid and access mode. Each named rendezvous
   sends that original descriptor. [Limbo owner](../../tests/flashvm/cancel-int-ref_pair.b) ·
   [Reference port](../../reference/module_ref_port.md).
2. The [AS2 task](../../tests/flashvm/cancel-int-ref_worker.as) receives a
   live alias on `os.chan`. A numeric read fetches the current value.
   Setting the alias writes to the owner. Each task has an independent
   context and releases its channel and fids on teardown.
3. The [test relay](../../tests/flashvm/int_ref_gate.py) withholds one
   original response per task. The stages are the stat before open,
   open, stat of the opened fid, read and write. The
   [movie](../../tests/flashvm/cancel-dis-setup_pair.as) reads an
   independent file in the same session before asking the task to stop.
   The relay requires a flush for the original tag and the clunk of the
   exact fid. The gate only controls acknowledgement timing in this test.
4. When the write reply is withheld, the Limbo owner already holds
   `42`. The next task opens the original reference again, reads `42`
   and replaces it with `73`. The relay checks the two assignments and
   their order. The owner checks its final value before closing the
   reference and unpublishing the variable. A failed transfer or missing
   recovery marker fails the test and the launcher reaps both runtimes.

Run the same command with `--modref-reverse` in place of `--cancel-int-ref`
to check publication replacement. Limbo keeps old aliases across a
replacement at the same path. Those aliases stop working, and a new
descriptor with a new Qid reads `77`.
[Replacement fixture](../../tests/flashvm/modref-reverse_pair.b).

## Use the same rule from C

This function checks the original Qid again and returns the visible value
only after a complete read. The caller supplies an attached session for
the original owner, the decoded descriptor and a stop callback. Use a
fresh callback state for a separate recovery attempt.

```c
#include <stddef.h>
#include <styxrpc/module_ref.h>

/* Borrow the session, descriptor and stop state through the call. */
static StyxRpcIoOutcome
read_export(struct StyxClient *issuer, const StyxRpcModuleRef *ref,
    int32_t *value, StyxRpcVfsStop stop, void *state)
{
    StyxRpcVfsFile *file = NULL;  /* The successful open transfers ownership. */
    StyxRpcIoOutcome status = styxrpc_module_ref_open_remote_until(
        issuer, ref, &file, stop, state);
    if (status == STYXRPC_IO_OK)
    {
        status = styxrpc_vfs_int_read_until(file, value, stop, state);
        styxrpc_vfs_close(file, 1);  /* Release locally and allow deferred clunk. */
    }
    return status;  /* The read only changes *value on success. */
}
```

If a [write](../../reference/vfs_int_write_until.md) returns
`STYXRPC_IO_UNCERTAIN`, close its fid and decide in the application
whether a read of the visible value is needed. Avoid replaying the
assignment. Keep the connection alive while operating on one of its fids.
[Exact open](../../reference/module_ref_open_remote_until.md) · [Cancellable read](../../reference/vfs_int_read_until.md) · [Close](../../reference/vfs_close.md).
