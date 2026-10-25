# Владение потоками и каналами

Одновременные [`fvm_worker_join`](../include/flashvm/worker.h#L60) одного живого handle сериализуются. Self-join
возвращает [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Состояние FINISHED публикуется после
уничтожения приватного контекста и его финализаторов. Destroy выполняет один
владелец: join, завершение остальных пользователей handle, destroy. Кооперативная
отмена не прерывает произвольный нативный callback.

`fvm_channel_receive_owned(channel, &value, blocking)` передаёт выделенную строку
потребителю. Освобождать один раз через [`fvm_channel_message_release`](../include/flashvm/channel.h#L126). Значение
не инвалидируется следующим receive или уничтожением канала. Это API для
нескольких потребителей строк. Старый receive возвращает заимствованную строку
и требует внешней синхронизации до завершения её чтения. `try_send` возвращает
[`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L49) при полной открытой очереди. Close будит отправителей и
получателей; отправка возвращает [`FVM_ERROR`](../include/flashvm/flashvm.h#L45), оставшиеся сообщения можно прочесть.
Перед передачей handle другому владельцу нужен retain; каждый активный
пользователь удерживает ссылку до завершения операций.

Теги примитивных значений проверяются до создания worker и отправки в канал.
Указатели объектов не передаются. Проверен Linux; остальные адаптеры требуют
нативного CI. Проверка идентичности потока и сон находятся в `ports/`; точность
сна зависит от ОС, Windows округляет вверх до миллисекунд.

`worker_join_concurrent` синхронизирует двух joiner через condition variable и
проверяет self-join без предположений о задержках. `channel_owned` проверяет
независимое время жизни строк, заполнение очереди, close и освобождение после
уничтожения канала.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_message_release` | function | [include/flashvm/channel.h:126](../include/flashvm/channel.h#L126) |
| `fvm_channel_receive_owned` | function | [include/flashvm/channel.h:122](../include/flashvm/channel.h#L122) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../include/flashvm/flashvm.h#L49) |
| `fvm_worker_join` | function | [include/flashvm/worker.h:60](../include/flashvm/worker.h#L60) |
<!-- source-links:end -->
