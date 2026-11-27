# FlashVM module variables in Limbo

[Русский](STYX-MODULE-REFS.ru.md) · [Interface](../module/styxmodref.m) · [Implementation](../limbo/styxmodref.b) · [C SDK](../../lib/styxrpc/reference/module_ref_port.md)

A program in the mounted DisVM and FlashVM pair can open a live signed
Int32 exported by FlashVM. `Styxmodref` returns a typed `ref Cell`.
The cell retains the original Styx mount and the export's Qid. Its
path is relative to that mount, independent of the Limbo program's
working directory and the mount's visible name in the guest namespace.

## Send and return a reference

Include `sys.m`, `styxnamed.m` and `styxmodref.m` in the Limbo application.
Load the matching `styxnamed.dis` and `styxmodref.dis` and initialize
them before opening a channel. The regular DisVM build provides both
modules. The ordinary `--styx-mount-fd`, `--styx-mount-tcp` and
`--styx-local-pair` session supplies their connection.

After FlashVM publishes `counter` as Int32 in `ref-pair.dis`, the
Limbo program captures the file with write permission and sends its
descriptor over an ordinary named channel.

```limbo
pipe := named->open("/module-refs");
cell := refs->capture("mounted/ref-pair.dis/.var/counter", 1);
pipe.send(cell.payload());
returned := refs->adopt(pipe.recv());
pipe.close();
after := returned.read();
returned.write(42);
returned.close();
cell.close();
```

The FlashVM movie joins the same channel after the capture. It receives
an alias of its export, replaces the value and sends back the same
reference. When the frame returns, the player can serve Limbo's next
read. In the Limbo example, `after` is `-73` and the final write sets
the original counter to `42`.

```actionscript
var pipe = vfs.chan("/module-refs");
var alias = pipe.recv();
alias = 0 - 73;
var sent = pipe.send(ref alias);
var closed = pipe.delete();
```

Use mode `0` when capturing a read-only reference. To forward it,
send the same `payload()` and keep its mode. `adopt` checks the
connection scope and defers opening the export until the first value
operation. Keep the owner available to process VFS requests during
`capture`, `read` and `write`. A withdrawn or replaced publication
invalidates its references, including an adopted handle that has yet to
open its fid. Applications can keep a reference after closing their
channel. Serialize operations on each individual cell.

## Acceptance

Run the [paired test command](STYX-CHANNEL.md#acceptance) with `--modref`.
It builds the [Limbo application](../../lib/styxrpc/tests/flashvm/modref_pair.b)
and [ActionScript movie](../../lib/styxrpc/tests/flashvm/modref_pair.as)
with the project compilers and linker. Expect
`PASS native modref RPC paired VMs`. The test writes through both
VMs, verifies the scope and permissions, then replaces the publication
under the same name. Add `--tcp` for direct TCP and `--jit` for JIT.

## Reference

- [init](styx-modref-api/init.md), [capture](styx-modref-api/capture.md), [adopt](styx-modref-api/adopt.md).
- [Cell.read](styx-modref-api/read.md), [Cell.write](styx-modref-api/write.md), [Cell.payload](styx-modref-api/payload.md), [Cell.close](styx-modref-api/close.md).
