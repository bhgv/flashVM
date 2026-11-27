# task.poll()

[English](ASINC_POLL.md) · [Руководство](ASINC_TASK.ru.md) ·
[Реализация](../src/standard/async_tasks.cpp)

Вызовите без аргументов на объекте задачи в контексте-владельце. Возвращает
новый локальный объект с логическим `done`, числовым `status` и `value`.
До завершения `done=false`, `status=FVM_NOT_FOUND` и `value=undefined`.
После завершения `done=true`, `status` содержит
[`FVM_Result`](../include/flashvm/flashvm.h#L42) рабочего потока, а `value`
содержит копию примитивного результата при успехе или `undefined` при
ошибке. Сохранённый объект результата удерживает значение после освобождения
задачи. Вызов не ждёт завершения потока. Повторный опрос допустим.

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
