# Mixed UniBus channels in Limbo

[Русский](STYX-CHANNEL.ru.md) · [Module interface](../module/styxchannel.m) · [Limbo implementation](../limbo/styxchannel.b) · [Flash provider](../../flashVM/doc/STYX_RPC_LISTEN.md)

Load `Styxchannel` from `styxchannel.dis` and call `init()` before use.
This fills in the mounted-session Unibus interface, the Math bit conversion
functions and the typed Int32 reference adapter. The standard runtime build
includes the Limbo library. The native builtins require a bidirectional
`--styx-mount-fd`, `--styx-mount-tcp` or `--styx-local-pair` session.
Transfers use the same session as the peer mount.

| Method | Contract |
| --- | --- |
| `newchan(address, root)` | Opens one named exchange at the peer attachment root. Use one nonempty name up to 255 UTF-8 bytes, optionally prefixed by "/". The absolute `root` must be the matching issuer mount when receiving references. Returns an owned `ref Port` or raises on refusal. |
| `port.send(message)` | Sends a `ref Message` and returns 1 after the Styx write is acknowledged or 0 on backpressure before transmission. Raises for invalid messages or transport failure. A failed write has an uncertain remote effect. |
| `port.recv()` | Returns the oldest complete `ref Message` or nil after an empty poll. Invalid or failed reads raise. A partial message can require more polls. |
| `port.close()` | Retires the port and discards unread messages. Repeated close succeeds. The guest root has up to eight open ports, all closed at launch teardown. |

Work on the mounted launch's original process. The host copies message
bytes across the boundary and runs Styx I/O outside managed heap admission.
Calls can wait for their Styx request deadlines.

`Message` is a Limbo pick ADT. `Integer`, `Floating`, `Boolean`,
`Text` and `Null` carry a snapshot. In `Variable` the cell is a typed
`ref Styxintref->Cell` in the issuer mount. Read it with
`cell.read()` and write with `cell.write(value)`. Forward only within
the same issuer path and keep that mount bound to the original session.
Its independently opened fid outlives the exchange port. The provider
must already have issued a live Int32 capability. A FlashVM movie
serving `.rpc` currently handles snapshots.

## Acceptance

[Limbo client](../../lib/styxrpc/tests/flashvm/mixed_pair.b) uploads and
starts the [AS program](../../lib/styxrpc/tests/flashvm/mixed_pair.as)
through the playback service and echoes five message kinds over one port.
The client also checks ordinary VFS visibility, duplicate open refusal and
reopening after close.

From the repository root, point `build` at a complete paired workspace
build directory. The driver compiles both applications using the project's
Limbo compiler, AS2 View, FIR and SWL. It starts the regular DisVM and
FlashVM executables. No application-specific native executable is required.

```sh
python3 lib/styxrpc/tests/flashvm/mixed_pair.py --source disVM \
  --runtime "$build/disvm-editor/disvm" \
  --player "$build/paired-flashvm/flashvm_styx_player" \
  --limbo "$build/disvm-editor/bootstrap/limbo-bootstrap" \
  --as2 "$build/as2/view/as2_viewc" --fir "$build/fir/fir1" \
  --fir-opt "$build/fir/fir1_opt" --swl "$build/linker/swl"
```

Expect `PASS native mixed RPC paired VMs`. Use `--tcp` for a native
loopback TCP pair and `--jit` to run both VMs with JIT. The independent SDK
checks `styxrpc-mixed-ref-disvm-interpreter` and `styxrpc-mixed-ref-disvm-jit`
use the [typed reference guest](../../lib/styxrpc/tests/flashvm/mixed_ref.b)
and verify the value at the reference issuer.
