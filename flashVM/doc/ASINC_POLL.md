# task.poll()

[Русский](ASINC_POLL.ru.md) · [Detached task guide](ASINC_TASK.md) ·
[Implementation](../src/standard/async_tasks.cpp)

Call without arguments on the owner's task handle. Returns a new local object
with Boolean `done`, numeric `status`, and `value`. Before completion the
result has `done=false`, `status=FVM_NOT_FOUND` and `value=undefined`.
On completion `done=true`, `status` is the worker's
[`FVM_Result`](../include/flashvm/flashvm.h#L42), and `value` is the copied
primitive result on success or `undefined` on failure. Retaining the result
object keeps the value after the task handle is dropped. The call never
waits for worker completion. Repeated polls are allowed.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../include/flashvm/flashvm.h#L49) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
