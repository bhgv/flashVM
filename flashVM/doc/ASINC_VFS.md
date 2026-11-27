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
control requests while the task waits for a read. Concurrent RPC calls
can still wait on the shared Styx client. A task's `cancel()` is
cooperative and can wait for a pending request to return or time out.
The paired player retains the connection through task teardown.

This interface covers ordinary files. File `load()`, named channels,
node and module publication and playback control stay with the movie.
A standalone context using [`fvm_async_install`](../include/flashvm/async.h#L15) alone has no mounted
worker VFS.

With the standard [pair test command](../../disVM/runtime/STYX-CHANNEL.md),
choose `--asinc-vfs`, optionally with `--tcp` and `--jit`. The Limbo
provider leaves the read pending until the film advances two more frames,
then prepends a union branch before replying. The write reaches that branch.
The test also checks hidden service names and independent cwd.
[Worker fixture](../../lib/styxrpc/tests/flashvm/asinc-vfs_worker.as) ·
[Limbo driver](../../lib/styxrpc/tests/flashvm/asinc-vfs_pair.b).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_async_install` | function | [include/flashvm/async.h:15](../include/flashvm/async.h#L15) |
<!-- source-links:end -->
