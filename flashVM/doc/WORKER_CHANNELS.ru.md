# Взаимодействие потоков через каналы (C89 API)

Каждый [`FVM_Worker`](../src/capi/worker_c.c#L14) создаёт независимые Runtime и Context в системном потоке. Первым аргументом C-функция получает свой контекст. Передавать указатели на обычные объекты AVM1 между контекстами нельзя. Вместо этого приложение может передать указатели на созданные отдельно [`FVM_Channel`](../src/capi/channel_c.c#L30) через `userdata` функции потока. Эти указатели должны оставаться действительными до завершения `join`.

## Сообщения, время жизни и ошибки

[`fvm_channel_send`](../include/flashvm/channel.h#L106) копирует примитивные значения и строки UTF-8, заканчивающиеся нулём; объекты отклоняются. [`fvm_channel_receive`](../include/flashvm/channel.h#L110) блокируется до появления сообщения или закрытия канала. [`fvm_channel_try_receive`](../include/flashvm/channel.h#L133) не блокируется: [`FVM_OK`](../include/flashvm/flashvm.h#L44) — сообщение получено, [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L49) — канал открыт и пуст, [`FVM_ERROR`](../include/flashvm/flashvm.h#L45) — канал закрыт и пуст. Строка после получения принадлежит каналу и действует до следующего успешного чтения из того же канала либо до уничтожения канала. Один канал — FIFO-очередь, а не пара концов соединения; для независимого обмена в двух направлениях используются два канала.

[`fvm_worker_stop`](../include/flashvm/worker.h#L69) только запрашивает остановку и не прерывает заблокированный `receive`. Чтобы разбудить такой поток, владелец должен закрыть соответствующий канал перед `join`. Уничтожать канал, пока другой поток им пользуется, нельзя. Автоматическое управление временем жизни общих каналов ещё не реализовано.

Рабочий пример C89 находится в `examples/embedding/worker_channels/`. Синтаксис потоков и каналов ActionScript пока запланирован, но не реализован.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:30](../src/capi/channel_c.c#L30) |
| `fvm_channel_receive` | function | [include/flashvm/channel.h:110](../include/flashvm/channel.h#L110) |
| `fvm_channel_send` | function | [include/flashvm/channel.h:106](../include/flashvm/channel.h#L106) |
| `fvm_channel_try_receive` | function | [include/flashvm/channel.h:133](../include/flashvm/channel.h#L133) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../include/flashvm/flashvm.h#L49) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Worker` | struct | [src/capi/worker_c.c:14](../src/capi/worker_c.c#L14) |
| `fvm_worker_stop` | function | [include/flashvm/worker.h:69](../include/flashvm/worker.h#L69) |
<!-- source-links:end -->
