# Live module variables in a DisVM and FlashVM pair

[Русский](STYX-MODULE-REFS.ru.md) · [Interface](../module/styxmodref.m) · [Implementation](../limbo/styxmodref.b) · [C SDK](../../lib/styxrpc/reference/module_ref_port.md)

A program in the mounted pair can relay a live signed Int32 export of
either VM through a named channel. `Styxmodref` returns a typed
`ref Cell` to Limbo. FlashVM uses transparent ActionScript aliases.
The reference retains the original export identity and access mode.

## Export owned by FlashVM

Include `sys.m` and `styxnamed.m` in the Limbo application.
`styxnamed.m` includes the module reference declarations.
Load the matching `styxnamed.dis` and `styxmodref.dis` and initialize
them before opening a channel. The regular DisVM build provides both
modules. The ordinary `--styx-mount-fd`, `--styx-mount-tcp` and
`--styx-local-pair` session supplies their connection.

After FlashVM publishes `counter` as Int32 in `ref-pair.dis`, the
Limbo program captures the file with write permission and sends its
descriptor over an ordinary named channel.
The path is relative to the original attachment, independent of the
working directory and the mount's visible name in the guest namespace.

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

`Processctl.cancelwait(pid)` can stop an active capture, exact open, read
or write on `/.varref` in a direct child. An interrupted call raises an
exception and the affected remote fid retires. The bound port retains its
original descriptor and checks the full Qid on the next access. An accepted
assignment can remain visible.
[Runnable Limbo and FlashVM example](../../lib/styxrpc/examples/disvm/REF_PORT_CANCELLATION.md).

## Export owned by DisVM

Use `nodes->publishat("/mounted/counter.dis/.var/value", "rw", big 4)`
from `Styxnodes` to publish an Int32 after preparing the parent
directories. Use `"r"` for a read-only export. The owner handles four
bytes at offset zero, in signed big-endian order. Serve those requests
in a separate Limbo thread when the main program also reads or writes
through a returned alias.

Capture the local export from Limbo as soon as its publication is ready.
The path starts at the original exported root. A typed receive returns
`Message.Modref` with the adopted cell, whose first value operation checks
that the same path and Qid still identify the export.

```limbo
pipe := named->open("/dis-module-refs");
cell := refs->capturelocal("mounted/counter.dis/.var/value", 1);
pipe.sendvalue(ref Message.Modref(cell));
pick returned := pipe.recvvalue() {
Modref =>
    updated := returned.reference.read();
    returned.reference.close();
}
cell.close();
pipe.close();
```

```actionscript
var pipe = vfs.chan("/dis-module-refs");
var alias = pipe.recv();
alias = 42;
var sent = pipe.send(ref alias);
var closed = pipe.delete();
```

A same-name replacement has a different Qid. Withdrawal prevents access
through both held and deferred references. The owner's fd close can
complete asynchronously. Check that its old entry is gone before reusing
its name. A FlashVM receiver can also originate a reference by opening a
remote DisVM export with `vfs.intVariable()` and sending `ref proxy.value`.

## Acceptance

Run the [paired test command](STYX-CHANNEL.md#acceptance) with `--modref`.
It builds the [Limbo application](../../lib/styxrpc/tests/flashvm/modref_pair.b)
and [ActionScript movie](../../lib/styxrpc/tests/flashvm/modref_pair.as)
with the project compilers and linker. Expect
`PASS native modref RPC paired VMs`. The test writes through both
VMs, verifies the scope and permissions, then replaces the publication
under the same name. Add `--tcp` for direct TCP and `--jit` for JIT.

Use `--modref-reverse` to test the DisVM-owned export. The
[Limbo owner](../../lib/styxrpc/tests/flashvm/modref-reverse_pair.b) and
[AS movie](../../lib/styxrpc/tests/flashvm/modref-reverse_pair.as) exchange
read-only and writable references, test both Int32 limits, and verify
the new Qid after withdrawal. Expect `PASS native modref-reverse RPC paired VMs`.

## Reference

- [init](styx-modref-api/init.md), [capture](styx-modref-api/capture.md), [capturelocal](styx-modref-api/capturelocal.md), [adopt](styx-modref-api/adopt.md).
- [Cell.read](styx-modref-api/read.md), [Cell.write](styx-modref-api/write.md), [Cell.payload](styx-modref-api/payload.md), [Cell.close](styx-modref-api/close.md).
