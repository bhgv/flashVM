# vfs.localChannel(shape, slots)

[English](ASINC_LOCAL_CHANNEL.md) · [VFS в задачах](ASINC_VFS.ru.md) ·
[Реализация](../src/styxrpc/channel_local.c)

Вызывайте `vfs.localChannel(shape, slots)` или `os.localChannel(shape, slots)`
из отдельной задачи штатного парного плеера. Метод создаёт локальный
канал для аргумента скомпилированной функции Limbo. Тип элемента должен
совпадать со схемой удалённого метода. Для `chan of int` укажите `"i"`,
для `chan of string` — `"s"`. Число слотов целое от 0 до 8. Ноль задаёт
синхронный канал. Неверная схема или ёмкость даёт [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47).
При нехватке памяти возвращается [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46).

Возвращённый объект принадлежит задаче. `trySend(value)` пытается
поставить элемент в локальную очередь и сообщает об успехе. Метод
`tryReceive()` забирает элемент или возвращает `undefined` при пустой
очереди. `close()` закрывает очередь целиком и допускает повторный вызов.
Пока DisVM заимствует очередь, операции с ней идут через встречный
контекст этого вызова. Перед возвратом вызов завершает заимствование.
Задача может ждать ответа, пока фильм обслуживает встречные запросы Styx.

[Тестовая задача AS](../tests/styxrpc/peer-asinc-scope-worker.as) заполняет
один слот, передаёт очередь методу Dis и получает ответ из канала DisVM.
Для именованного обмена с динамическими значениями используйте
[`vfs.chan(path)`](ASINC_VFS.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
