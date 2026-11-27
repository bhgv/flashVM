# Async.run(function)

[Русский](ASINC_RUN.ru.md) · [Detached task guide](ASINC_TASK.md) ·
[Implementation](../src/standard/async_tasks.cpp)

Pass one same-context, serializable AVM1 function. The call snapshots its
environment synchronously and returns a task object after starting an OS
worker. The worker calls the snapshot without arguments in an independent
runtime. The copied graph has no pointer into the launching heap. Updates
made by one context do not change the other. Serialization and thread
creation errors are reported at launch. Unsupported entries include
native functions, functions with an attached native body, and captures
containing resources for which a detached copy is unavailable. A context
retains up to 16 tasks.

The owner can read primitive results through [poll](ASINC_POLL.md) and
request [cancellation](ASINC_CANCEL.md). If a worker returns an object,
its completion fails. The task does not share the caller's VFS session
or UI. Embed with [`fvm_async_install`](../include/flashvm/async.h#L15).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_async_install` | function | [include/flashvm/async.h:15](../include/flashvm/async.h#L15) |
<!-- source-links:end -->
