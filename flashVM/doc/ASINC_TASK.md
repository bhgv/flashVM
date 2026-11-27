# Detached AS2 tasks on OS threads

[Русский](ASINC_TASK.ru.md) · [Compiler profiles](../../compiler/ASINC.md) ·
[Owner-loop posts](ASYNC.md) · [C worker API](WORKER_FUNCTION.md)

The project AS2 view compiler accepts a complete unit with one closed
function. Compile it with the usual AS2 view, FIR and SWL tools.

```actionscript
asinc function compute() {
    var checksum = 40;
    return checksum + 2;
}
```

The SWF initializer installs `_root.compute`. Calling `_root.compute()`
creates an independent FlashVM context on an OS thread and returns a task.
The parent session and window can keep running during this computation.
The new context runs the detached bytecode with its own standard library.
Only previously declared `var` bindings, up to 64 names, and the worker's
`Math` namespace are visible to the compiled profile. A call takes zero
arguments. The profile rejects outer bindings, `ref`, parameters and other
`asinc` forms. Shared capture publication and live VFS access from tasks
remain separate work.

## Task operations

| Operation | Result |
| --- | --- |
| [`Async.run(function)`](ASINC_RUN.md) | Copy a serializable AVM1 closure and start its body on a worker. |
| [`task.poll()`](ASINC_POLL.md) | Read one completion snapshot without waiting for the worker or for a frame. |
| [`task.cancel()`](ASINC_CANCEL.md) | Request cooperative cancellation at a VM execution checkpoint. |

The compiler installs its closed template with `Async.wrap`. This helper
is part of the compiler/player contract and copies the function's bytecode
and constants without its originating timeline or closure. The native
body of an imported function cannot be transferred. A context can retain
up to 16 tasks, including released workers that still need cleanup.
Collecting a task handle requests cancellation without joining it from GC.
The owner reaps on a later run, timer poll or context destruction. Context
destruction requests stops and waits for the last retained workers to exit.

The paired player installs the namespace before executing the SWF. An
embedding host can call [`fvm_async_install`](../include/flashvm/async.h#L15).
The project [peer test](../../ide/tests/check_flash_peers.py) compiles the
[worker unit](../tests/styxrpc/peer-asinc.as) and the
[caller](../tests/styxrpc/peer-asinc-frame.as) and executes both in the stock
player. Use `--asinc-only` with the normal player, AS2, FIR and SWL paths.
Use `--tcp` to exercise TCP in place of the local socket pair. The
[native test](../tests/asinc_tasks.cpp) verifies that a looping worker can
be polled and stopped while the calling context remains active, and that
the returned string retains its exact bytes after the worker ends.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_async_install` | function | [include/flashvm/async.h:15](../include/flashvm/async.h#L15) |
<!-- source-links:end -->
