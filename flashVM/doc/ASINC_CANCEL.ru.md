# task.cancel()

[English](ASINC_CANCEL.md) · [Руководство](ASINC_TASK.ru.md) ·
[Реализация](../src/standard/async_tasks.cpp)

Вызовите без аргументов, чтобы запросить кооперативную остановку. Метод сразу
возвращает `undefined` после записи запроса. Поток, увидевший запрос в
контрольной точке исполнения, завершается с [`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Завершённая задача
сохраняет прежний результат. Запрос можно повторять. Узнать итог можно через
[poll](ASINC_POLL.ru.md). Принудительного завершения системного потока нет.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
<!-- source-links:end -->
