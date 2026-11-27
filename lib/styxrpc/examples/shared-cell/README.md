# Share a cell between C and Limbo

[Русский](README.ru.md) · [Example catalog](../README.md) · [Cell concepts and API index](../../reference/named_cell.md)

Use this example to pass a writable reference across a Styx attachment.
A C process hosts `/.channels` and opens a private local mount of the same
registry. A Limbo program runs in DisVM and mounts the remote end of a
Unix socket. Each program opens its own fid to the same cell and can read
the other's confirmed updates. The named channel transfers the reference
and orders the steps. The cell itself remains usable through its lease
after the transfer completes.

## Build and run

Use Linux64, a C compiler, CMake, Python 3 and the installed hosted Styx
and Unibus packages. The C provider uses Linux `getrandom` for capability
entropy. You also need a built DisVM, its Limbo bootstrap compiler, both
module include directories and the Limbo sources for `Styxnamed` and
`Styxvalue`. The C build needs only the standalone libstyxrpc directory
and the SDK prefix.

```sh
cmake -S /path/to/styxrpc -B /path/to/rpc-build \
    -DCMAKE_PREFIX_PATH=/path/to/sdk -DSTYXRPC_HOSTED=ON -DSTYXRPC_EXAMPLES=ON
cmake --build /path/to/rpc-build --target styxrpc-shared-cell

python3 /path/to/styxrpc/examples/shared-cell/run.py \
    --host /path/to/rpc-build/styxrpc-shared-cell \
    --disvm /path/to/dis-build/disvm --limbo /path/to/dis-build/bootstrap/limbo-bootstrap \
    --system-modules /path/to/disVM/upstream/module --native-modules /path/to/disVM/module \
    --limbo-sources /path/to/disVM/limbo
```

| Option | Use |
| --- | --- |
| `--host` | Executable of the C provider just built. |
| `--disvm` | DisVM runtime with the raw Styx attach and native thread launch commands. |
| `--limbo` | Bootstrap compiler for the Limbo application and its libraries. |
| `--system-modules` | Directory containing `sys.m` and the standard Limbo declarations. |
| `--native-modules` | Project module declarations, including `styxnamed.m`. |
| `--limbo-sources` | Directory containing `styxvalue.b` and `styxnamed.b`. |
| `--jit` | Use DisVM's JIT. The default runs its interpreter. |

The [launcher](run.py) compiles all three `.dis` files into a temporary
working directory. It gives each process one end of the socket, mounts
the guest end at `/device` with `--raw`, and runs the guest on a native
thread. The guest places the mounted service first in its root namespace
so `Styxnamed` finds `/.channels`. The host sets an eight-second application deadline alongside Styx request limits.
The launcher bounds child waits and removes the temporary images.

Expected output in either mode.

```text
PASS C shared cell cancellation and token retirement
PASS Limbo observed both values through the same lease
PASS Dis application completed
```

## Follow the exchange

| Step | C provider | Limbo guest |
| --- | --- | --- |
| 1 | Create a cell holding Boolean `true`. Send tag 8 followed by its 24-byte token on `/settings`. | Receive the descriptor and attach an independent lease before the next channel operation. Read `true`. |
| 2 | Receive `a`, attach another C lease to the same token, then close the creator's fid. | Write Boolean `false` and send notification `a`. |
| 3 | Request stop before writing. Read `false` through a temporary fid. Then write `true` with the normal deadline and send `r`. | After `r`, read `true` through the same Limbo lease. |
| 4 | Receive `c`, close the remaining C lease, then wait for attachment with the old token to be refused. | Close the Limbo cell lease and send `c`. |
| 5 | Send `d`, wait for `e`, close the channel, join the service owners and release the registry. | Receive `d`, send `e`, observe the host disconnect and close the channel. |

The [C source](host.c) separates the instruction rendezvous from value
access. `exchange()` keeps the first lease through transfer, then closes
it after both aliases exist. `reads_disabled()` borrows the token, verifies
the tag and Boolean and frees the owned reply. `deadline_reached()`
supplies the caller's monotonic stop hook. `already_stopped()` selects
the early stop deterministically.

The [Limbo source](guest.b) attaches the descriptor while the named
receive still holds a transfer pin. The independent lease survives
later channel messages. Its close precedes `c`. The final `e` confirms
Limbo consumed the complete `d` frame. Limbo stays attached so C can
read `e` before shutting down the service.
The guest can release its FD before the provider receives the clunk.
`retired_token()` probes using fresh fids until it sees refusal or the
application deadline. It closes a successful probe immediately. After a
refusal, the final rendezvous confirms that the attachment is still live.


## Adapt the example

Use the early stop when an assignment is no longer needed. In a live
application cancellation can occur after sending part or all of a value.
A write without positive acknowledgement can return
`STYXRPC_IO_UNCERTAIN`. Read the current frame through a live lease
when the next application decision needs it. Other writers may have
changed it. The [wire test](../../tests/cell_stop_wire.c) covers the
cancelled header, a submitted body, short confirmed prefix and reply
precedence. The [paired FlashVM acceptance](../../tests/flashvm/cancel-cell-setup_worker.as)
shows cancellation while accessing a received reference from ActionScript.

When the example fails, the launcher reports both process outputs.
A mount or missing module error points to the runtime and include paths.
A `shared-cell exchange, deadline or retirement check failed` message
means the C peer could not establish the expected ordered exchange.
Either process can exit after a timeout. The launcher closes the socket and
reaps the processes, allowing the provider to release outstanding fids.

[Create](../../reference/cell_create_until.md) ·
[Attach](../../reference/cell_attach_until.md) ·
[Read](../../reference/cell_read_until.md) ·
[Write](../../reference/cell_write_until.md) ·
[Start the two-mount host](../../reference/named_peer_host.md) ·
[Close a fid](../../reference/vfs_close.md)
