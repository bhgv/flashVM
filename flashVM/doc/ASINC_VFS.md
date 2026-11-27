# Mounted VFS in AS2 tasks

[Русский](ASINC_VFS.ru.md) · [AS2 task guide](ASINC_TASK.md) ·
[File methods](STYX_RPC_OS.md) · [Paired player](STYX_RPC_PLAYER.md)

In the paired player, an AS2 `asinc` function and an `Async.run` worker
can use `vfs` and `os`. Both names refer to one object created in the
worker's own context. Its current directory starts at `/`. Changing it
leaves the film's current directory and other tasks' directories intact.

`open`, `opendir`, `attr`, `mkdir`, `rm`, `cd` and `pwd` use the
same mounted ordinary namespace as the film. The usual file and
directory handles close explicitly or at worker teardown. Paths are
resolved through libstyxrpc, with the same service-name filtering. A
task can read and write files on the peer. For example:

```actionscript
asinc function firstByte(path) {
    var file = vfs.open(path);
    var bytes = file.read(1);
    var closed = file.close();
    return bytes[0];
}
```

The task runs on a separate OS thread. The movie can advance and serve
control requests while the task waits for a read. For `file.read(count)`,
[`cancel()`](ASINC_CANCEL.md) sends `Tflush` for an outstanding data
request and closes the exact file fid if cancellation wins. The read ends
with an error and the task publishes failed completion. The paired DisVM
server defers reads from interruptible providers, including positioned
Limbo callback files, so it can process the flush and unrelated fids
while the callback waits. File writes, metadata and directory reads
use their ordinary deadlines. The player retains the connection through
task teardown.

When the pair has negotiated named channels, `vfs.chan(path)` and
`os.chan(path)` open an independent enrollment for the task.
`send(value)` and `recv()` can wait while the movie advances. `cancel()`
flushes a pending data round trip and closes the enrollment. If a
rendezvous raced cancellation, the peer may have consumed the value.
The task can exchange snapshots and named cell descriptors. References
to module exports require owner processing.

A worker can also open a published `.dis` image for reading and use
`load()` to get a proxy in its own context. Closing the original handle
leaves the proxy's own fid available for direct method calls. The proxy
stays bound to that publication. Withdrawal makes it fail even if the
same VFS name is reused. The stock player assigns each active task a
distinct reverse PID and argument bus. A task can call scoped methods,
lend a [local typed queue](ASINC_LOCAL_CHANNEL.md) as an argument and
use a channel returned by DisVM through that task's proxy. Both directions
use the paired Styx session. The movie receives a primitive result through
`poll()`.

In a [live Dis session](STYX_RPC_LIVE_DIS.md), `cancel()` stops pending
image loads and method calls during data round trips. The task closes the exact function fid,
and checked scoped Dis code can unwind from a local channel wait.
Use [`os.unload(proxy)`](STYX_RPC_UNLOAD.md) to release a checked proxy
inside its task.

Loading a child SWF, publishing nodes or modules and controlling playback
use the movie's context.
A standalone context using [`fvm_async_install`](../include/flashvm/async.h#L15) alone has no mounted
worker VFS.

With the standard [pair test command](../../disVM/runtime/STYX-CHANNEL.md),
choose `--asinc-vfs`, optionally with `--tcp` and `--jit`. The Limbo
provider leaves the read pending until the film advances two more frames,
then prepends a union branch before replying. The write reaches that branch.
The test also checks hidden service names and independent cwd.
[Worker fixture](../../lib/styxrpc/tests/flashvm/asinc-vfs_worker.as) ·
[Limbo driver](../../lib/styxrpc/tests/flashvm/asinc-vfs_pair.b).
Use `--cancel-vfs-read` to verify confirmed-prefix cancellation on one
callback file while another read is pending. The
[Limbo driver](../../lib/styxrpc/tests/flashvm/cancel-vfs-read_pair.b)
checks that the cancelled ticket refuses a late reply and that a fresh
open can read the same file. The pair also shuts down with a read still
pending. Run with `--jit` and `--tcp` to exercise those modes.
The `--asinc-chan` profile checks echo, cancellation of unmatched send
and receive, exact name removal and disconnect with a task still pending.
Use `ide/tests/check_flash_peers.py --asinc-only` for two FlashVMs. Add
`--runtime` and `--limbo` to compile the Dis provider and check the worker
proxy in interpreter and JIT modes over TCP. The
[AS worker](../tests/styxrpc/peer-asinc-load-worker.as) closes the source
handle before calling `next`. The
[film](../tests/styxrpc/peer-asinc-load-caller.as) checks a value of `42`
and a remote rejection from another task. The
[scoped worker](../tests/styxrpc/peer-asinc-scope-worker.as) lends its
queue to the [Limbo export](../../lib/styxrpc/tests/flashvm/asinc-scope_owner.b),
then reads and closes the returned Dis channel. The film runs two such
calls concurrently and verifies both results.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_async_install` | function | [include/flashvm/async.h:15](../include/flashvm/async.h#L15) |
<!-- source-links:end -->
