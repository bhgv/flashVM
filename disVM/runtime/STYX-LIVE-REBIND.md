# Live namespace changes on a paired Styx session

[Русский](STYX-LIVE-REBIND.ru.md) · [ActionScript VFS](../../flashVM/doc/STYX_RPC_OS.md) · [Directory rewind](../../flashVM/doc/STYX_VFS_DIR_REWIND.md)

The paired DisVM publishes the namespace shared by the running Limbo
application. A Limbo thread in that same namespace group can change
bindings after FlashVM attaches. Fresh path lookups and file opens in
FlashVM resolve through the current mount order.

| Change on DisVM | Effect on fresh lookup |
| --- | --- |
| `Sys.bind(source, target, Sys->MAFTER)` | The new layer contributes names after the original directory. |
| `Sys.bind(source, target, Sys->MBEFORE)` | The new layer takes precedence for overlapping names. |
| `Sys.unmount(source, target)` | Remove the selected layer. Lower layers become visible. |
| `Sys.bind(source, target, Sys->MREPL)` | Resolve through the replacement layer. |
| `Sys.unmount(nil, target)` | Remove bindings on the target and reveal the original path. |

An already open file retains its original object and remains usable
while that object accepts operations. For a directory, the first
read at byte offset zero and each rewind enumerate the current view
of the held identity. If the original directory has been replaced,
open the pathname again to reach the replacement. A continuation
at a nonzero offset follows its existing cursor. This keeps the
buffered records of one read together. Directory lists omit reserved
service names such as `.prog`.

The [native directory reader](../native/vfs/tree.h#L41) checks the identity
of the refreshed cursor before replacing it. No new wire messages
or application-specific native host are required.

## Paired verification

Run the [paired build command](STYX-CHANNEL.md#acceptance) with
`--rebind`. It builds the [Limbo driver](../../lib/styxrpc/tests/flashvm/rebind_pair.b)
and [AS movie](../../lib/styxrpc/tests/flashvm/rebind_pair.as) using the
project compilers and linker. The movie opens the original file and
directory before sending its readiness signal. The Limbo worker
changes its cwd and applies the five namespace changes in the table.
The [AS callback](../../lib/styxrpc/tests/flashvm/rebind_signal.as)
carries the expected results over the already attached session.

The first read of the held directory sees the appended branch. Later
reads verify selected contents and live stat, a file from the fallback
branch, and the unchanged contents of the original file descriptor.
The driver prints `PASS native rebind RPC paired VMs` at completion.
Use `--tcp` and `--jit` to select the other supported modes.
