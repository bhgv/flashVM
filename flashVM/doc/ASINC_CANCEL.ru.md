# task.cancel()

[English](ASINC_CANCEL.md) · [Руководство](ASINC_TASK.ru.md) ·
[Реализация](../src/standard/async_tasks.cpp)

Вызовите без аргументов, чтобы запросить кооперативную остановку. Метод сразу
возвращает `undefined` после записи запроса. Поток, увидевший запрос в
контрольной точке исполнения, завершается с [`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Завершённая задача
сохраняет прежний результат. Запрос можно повторять. Узнать итог можно через
[poll](ASINC_POLL.ru.md). Принудительного завершения системного потока нет.

Парный плеер проверяет отмену, пока задача ждёт данные метода Dis, живой
загрузки, канала или [чтения либо записи файла](ASINC_VFS.ru.md).
Он отправляет `Tflush` для ожидающего чтения или записи Styx. Если отмена
победила, плеер закрывает fid операции. Запросы метаданных, подтверждение
flush и закрытие fid используют сроки сессии.
[Живые вызовы Dis](STYX_RPC_LIVE_DIS.ru.md) ·
[Возвращённые типизированные каналы](STYX_RPC_CHANNELS.ru.md)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
<!-- source-links:end -->
