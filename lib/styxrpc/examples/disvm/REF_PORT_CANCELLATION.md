# Cancel a Limbo wait on a FlashVM Int32 export

[Русский](REF_PORT_CANCELLATION.ru.md) · [Example index](../README.md) · [Reference port](../../reference/module_ref_port.md)

A Limbo application can receive a live Int32 reference and use it after
the named channel closes. The guest opens its local `/.varref` port.
That port owns the descriptor and uses the shared C SDK to resolve the
export on the original FlashVM mount. A direct parent in DisVM can use
`Processctl.cancelwait(pid)` when a child is waiting inside
`Styxmodref.capture`, `Cell.read` or `Cell.write`.

A cancelled value operation retires the affected remote fid. A later
read on the same port checks the entire original Qid and fetches the
current value. An accepted assignment can remain in effect after its
acknowledgement is lost. The two peers exchange the standard Int32
frames and use the existing 9P `Tflush` and clunk.

## Run the pair

Build native DisVM with terminal launch support, its Limbo bootstrap,
the StyxRPC-enabled FlashVM player, the AS2 frontend, FIR tools and
the SWF linker. See the [DisVM setup](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/STYX-CHANNEL.md)
and [FlashVM player guide](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_PLAYER.md).
Use the paths to your build products from the repository root.

```sh
python3 lib/styxrpc/tests/flashvm/mixed_pair.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --player build/flash/flashvm_styx_player \
    --as2 out/as2/as2_viewc --fir out/fir/fir1 \
    --fir-opt out/fir/fir1_opt --swl out/linker/swl \
    --cancel-ref-port
```

The launcher takes the DisVM source and modules from `--source`.
It builds the Limbo owner and adapter with `--limbo` and runs them
with `--runtime`. It compiles the FlashVM movie through the AS2
frontend, FIR checker and optimizer, and SWF linker before uploading
it to `--player`. The default uses the interpreter and Unix socket
pairs. `--jit` selects JIT, and `--tcp` uses loopback TCP.

Expect `PASS native cancel-ref-port RPC paired VMs`. The launcher
reaps both processes and closes its relay sockets. A missing flush,
unretired fid, replayed assignment or stale reference that succeeds
fails the test. With the normal DisVM integration inputs plus
`STYXRPC_FLASHVM_TOOLS` at the tool build root and `STYXRPC_FLASHVM_PLAYER`
at the player executable, run all four combinations using
`ctest --test-dir build/rpc -R styxrpc-native-ref-port --output-on-failure`.

## Follow the exchange

1. The [AS2 movie](../../tests/flashvm/cancel-ref-port_pair.as) publishes
   a writable counter initialized to `17`. The [Limbo parent](../../tests/flashvm/cancel-ref-port_pair.b)
   captures that Qid, copies the descriptor and adopts it into a port.
   It also creates a second, unopened alias for the replacement test.
2. In successive direct children, the parent interrupts the pre-open
   stat, open, the opened fid's stat, value read and value write. The
   [relay](../../tests/flashvm/int_ref_gate.py) holds the original reply
   on the forward lane. The parent uses a separate fid successfully
   before requesting each cancellation. The relay requires a flush of
   the held tag and an acknowledged clunk of the exact temporary fid.
3. The held write has already assigned `42` in FlashVM. The parent
   checks the original descriptor bytes, then reads `42` on the same
   Limbo port. It writes and verifies `73` as a new operation.
4. The parent restarts the movie and waits for the new Qid. The old
   bound and unopened references fail. A fresh capture reads `17`.
   The parent closes its ports. This verifies both identity and the
   independent state of the replacement.

## Use the port from C

The C embedding can pass a deadline instead of a guest cancellation
source. The `timespec` supplied below is an absolute, initialized
deadline on `CLOCK_MONOTONIC`. On a clock error, stop the wait.

```c
#define _POSIX_C_SOURCE 200809L  /* Expose clock_gettime on Linux. */
#include <time.h>
#include <styxrpc/module_ref_port.h>

static int deadline_reached(void *state)
{
    const struct timespec *deadline = state;  /* Borrow through return. */
    struct timespec now;                     /* Current monotonic time. */
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        return 1;  /* Stop when the deadline cannot be compared. */
    }
    return now.tv_sec > deadline->tv_sec ||
        (now.tv_sec == deadline->tv_sec && now.tv_nsec >= deadline->tv_nsec);
}
```

Call the [cancellable read](../../reference/module_ref_port_read_until.md)
or [cancellable write](../../reference/module_ref_port_write_until.md)
with that callback and a valid port. Serialize operations on the port.
After an uncertain assignment, use a fresh wait for the readback if the
application needs it. Other holders can still write in the meantime.
When finished, [close the port](../../reference/module_ref_port_close.md).
