# Cancel admission of delegated channels

[Русский](CHANNEL_ADMISSION.ru.md) · [SDK examples](../README.md) · [Channel leases](../../reference/channel_leases.md)

A native result can carry a channel owned by the same DisVM module or a
temporary PID on the reverse connection. The receiving owner confirms
the route before using it. The C SDK stages all routes from one frame in
a private table and admits them together. An interrupted confirmation
leaves previously admitted indices available. The original routes use
the same UniBus records and 9P requests in FlashVM, DisVM and the SDK.

| Address in the received frame | Where the SDK confirms it | What a successful import retains |
| --- | --- | --- |
| Module token and key | `.unibus` on the module pinned by the issuing result | The token, key, element schema and rendezvous flag in the lease |
| Reverse PID and key | The issuer's `.prog/<pid>/.pid` and `.unibus` on the original reverse mount | A root entry in the caller's PID table, with its key, schema and rendezvous flag |

## Set a deadline from C

Use a monotonic clock and supply one absolute deadline per import.
A deadline is checked on the calling thread, including while a
multiplexed request waits for its response. In the following helper,
`deadline` points to an initialized `timespec` on the same clock.
If the clock fails, the callback requests a stop. The mounted client's
existing request deadlines still apply.

```c
#define _POSIX_C_SOURCE 200809L  /* Make clock_gettime available on Linux. */
#include <time.h>
#include <styxrpc/channel_client.h>

static int deadline_reached(void *state)
{
    const struct timespec *deadline = state;  /* Borrow the caller's deadline. */
    struct timespec now;                     /* Current monotonic time. */
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        return 1;  /* Stop if a valid comparison is impossible. */
    }
    return now.tv_sec > deadline->tv_sec ||
        (now.tv_sec == deadline->tv_sec && now.tv_nsec >= deadline->tv_nsec);
}
```

On the result owner, call [module import](../../reference/channel_lease_import_message_until.md)
with the pinned lease, or [PID import](../../reference/channel_pid_import_until.md)
with a caller-owned table and the attached reverse session. Pass
`deadline_reached` and the address of the deadline. Use a new deadline
for a deliberate second attempt on application-retained frame bytes.
A FlashVM mailbox that has already removed a frame consumes that frame
even when its later confirmation is cancelled. A subsequent mailbox
read takes the next frame.

## Run the complete exchange

Build the native Linux64 DisVM terminal launch runtime and its
`limbo-bootstrap`, and configure the hosted SDK with
`STYXRPC_EXAMPLES=ON`. See the SDK's [build instructions](../../README.md)
and the [DisVM terminal setup](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/STYX-CHANNEL.md).
From a checkout with both projects, use the paths to your build products.

```sh
python3 lib/styxrpc/tests/flashvm/channel_admission.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --driver build/rpc/styxrpc-routed-client
```

| Argument | Role |
| --- | --- |
| `--source` | DisVM source directory containing the Limbo example and module interfaces |
| `--runtime` | Native DisVM with terminal launch support |
| `--limbo` | Limbo bootstrap compiler that builds the provider and dispatcher |
| `--driver` | C example executable linked to the independently built SDK |
| `--jit` | Select JIT in DisVM. The default uses its interpreter. |
| `--tcp` | Use loopback TCP for both relay legs. The default uses Unix socket pairs. |

The test prints `PASS atomic route import against native DisVM`.
The runner reaps both processes, closes the connected descriptors and
removes the temporary directory. A wrong acknowledgement, missing clunk
or failed channel operation causes a nonzero exit status.
Set `STYXRPC_DISVM_SOURCE` and `STYXRPC_DISVM_BUILD` when configuring
the SDK to register all four mode and transport combinations in CTest.
Run them with `ctest --test-dir build/rpc -R styxrpc-disvm-channel-admission --output-on-failure`.

## Follow the requests

1. The [C caller](routed_client.c) publishes the two reverse endpoints and
   invokes the [Limbo provider](https://github.com/bhgv/flashVM/blob/master/disVM/examples/remote-scalar/styx-channel-recursive-owner.b).
   The returned result supplies two routes on one lease. DisVM sends a
   new module channel through the first reverse endpoint.
2. The caller stages a tuple containing the new route and one existing
   alias. The [relay](../../tests/flashvm/channel_admission.py) passes the
   first token exchange. It withholds the second original `Rwrite`, from
   the real DisVM server, and signals the stop callback. The test
   checks `Tflush` for that original tag and the acknowledged clunk of
   its temporary bus fid.
3. The C caller observes `STYXRPC_CANCELLED` and checks that the existing
   index still selects the original route. The new index is absent.
   It imports the same frame with a fresh wait, finds the new index and
   uses the channel to send its second reverse PID. DisVM and C exchange
   `43` and `81`, and the C caller reads `81` on the original result.
4. Clunking the issuing result ends the lease. Both tokens then fail
   subsequent channel admission or exchange. The runnable scenario
   checks these exchanges and rejects a deliberately incorrect token.

The [reverse PID wire fixture](../../tests/channel_stop_wire.c) checks
the exact tag and fid during a stalled identity walk, open and read.
It also delivers the original `Rread` before `Rflush`. The import still
reports cancellation when stop wins before its route table is published.
An independent fid responds before and after the flush.
[FlashVM reverse PID scenario](../../tests/flashvm/channel_pid_result.b).
