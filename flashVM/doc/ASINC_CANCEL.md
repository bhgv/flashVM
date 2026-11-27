# task.cancel()

[Русский](ASINC_CANCEL.ru.md) · [Detached task guide](ASINC_TASK.md) ·
[Implementation](../src/standard/async_tasks.cpp)

Call without arguments to request cooperative stop. Returns `undefined`
as soon as the request is recorded. A worker that observes stop at an
execution checkpoint finishes with [`FVM_ERROR`](../include/flashvm/flashvm.h#L45). A task already completed
keeps its prior result. Repeated requests are harmless. Read completion with
[poll](ASINC_POLL.md). Cancellation does not force an OS thread to exit.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
<!-- source-links:end -->
