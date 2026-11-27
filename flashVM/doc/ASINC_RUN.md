# Async.run(function[, ...args])

[Русский](ASINC_RUN.ru.md) · [Detached task guide](ASINC_TASK.md) ·
[Implementation](../src/standard/async_tasks.cpp)

Pass a same-context, serializable AVM1 function followed by up to eight
positional values. The call snapshots the environment and arguments as one
graph and returns a task object after starting an OS worker. Aliases and
cycles supported by the graph codec survive in the copy. The worker calls
the function with those values in an independent runtime. The copied graph has no pointer into the launching heap. Updates
made by one context do not change the other. Serialization and thread
creation errors are reported at launch. Unsupported entries include
native functions, functions with an attached native body, and captures
containing resources for which a detached copy is unavailable. A context
retains up to 16 tasks.

The owner can read primitive results through [poll](ASINC_POLL.md) and
request [cancellation](ASINC_CANCEL.md). If a worker returns an object,
its completion fails. The paired player provides [ordinary VFS](ASINC_VFS.md) with a private working directory. The worker has no movie timeline. Embed with [`fvm_async_install`](../include/flashvm/async.h#L15).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_async_install` | function | [include/flashvm/async.h:15](../include/flashvm/async.h#L15) |
<!-- source-links:end -->
