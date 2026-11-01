# Detached AVM1 function workers

`fvm_worker_run_function(owner, config, function, &worker)` copies a first-class
AVM1 function and its serializable lexical environment, then starts one native
OS thread with its own runtime, context and heap. The worker invokes the restored
function without arguments. Launch does not execute its bytecode on the sender,
advance a Flash timeline or dispatch a callback on the owner loop.

Use this embedding primitive for isolated calculations. Mutating a captured
object changes the worker copy. Sender objects may be released after successful
launch. The existing graph codec preserves cycles and aliases within each copy.
The sender must call launch on its owning thread and root objects retained across
other allocations, as with the ordinary function API.

Launch serializes synchronously. Graph limits remain4096 objects,65536 entries
and64MiB; transport temporarily expands bytes into two UTF-8 letters per byte
because worker string inputs require valid UTF-8. This is not zero-copy or a
latency guarantee for large captures. There are no raw owner pointers in the
transport. No custom native callback registry is installed in the destination.
Unsupported captured resources can therefore fail serialization/restoration.
Native root entries, root native body attachments and live function-transfer
mode are explicitly refused. Restored bytecode does not inherit native/JIT body
attachments. Never use this to transfer live UI authority or expect native
attachments on nested captured functions to execute in the worker.

Execution failures arrive through [`fvm_worker_join`](../include/flashvm/worker.h#L75)/[`fvm_worker_result`](../include/flashvm/worker.h#L108) after a
successful launch. Only primitive results cross back; object results are refused.
Strings remain worker-owned until destruction. Interactive owners should poll
result and keep serving input, then join and destroy when complete. Instruction
budgets and cooperative worker cancellation use the existing execution machinery.

This is a foundation for executable `asinc`, not its language implementation.
The Coco/R/FIR closure lowering, dirty-only frame-boundary capture synchronization,
COW storage, variable-change listeners and complete IDE actor migration remain
open. `Async.post` continues to mean owner-loop delivery, not another OS thread.
No SWF or Dis64 file format changes are introduced.

[Public header](../include/flashvm/worker.h) ·
[Runnable example](../examples/embedding/worker_function/README.md) ·
[Regression](../tests/worker_function.c).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_worker_join` | function | [include/flashvm/worker.h:75](../include/flashvm/worker.h#L75) |
| `fvm_worker_result` | function | [include/flashvm/worker.h:108](../include/flashvm/worker.h#L108) |
| `fvm_worker_run_function` | function | [include/flashvm/worker.h:38](../include/flashvm/worker.h#L38) |
<!-- source-links:end -->
