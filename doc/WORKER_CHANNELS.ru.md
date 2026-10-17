# Взаимодействие потоков через каналы (C89 API)

Каждый [`FVM_Worker`](../src/capi/worker_c.c#L12) создаёт независимые Runtime и Context в системном потоке. Первым аргументом C-функция получает свой контекст. Передавать указатели на обычные объекты AVM1 между контекстами нельзя. Вместо этого приложение может передать указатели на созданные отдельно [`FVM_Channel`](../src/capi/channel_c.c#L26) через `userdata` функции потока. Эти указатели должны оставаться действительными до завершения `join`.

## Сообщения, время жизни и ошибки

[`fvm_channel_send`](../include/flashvm/channel.h#L53) копирует примитивные значения и строки UTF-8, заканчивающиеся нулём; объекты отклоняются. [`fvm_channel_receive`](../include/flashvm/channel.h#L55) блокируется до появления сообщения или закрытия канала. [`fvm_channel_try_receive`](../include/flashvm/channel.h#L70) не блокируется: [`FVM_OK`](../include/flashvm/flashvm.h#L42) — сообщение получено, [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L47) — канал открыт и пуст, [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) — канал закрыт и пуст. Строка после получения принадлежит каналу и действует до следующего успешного чтения из того же канала либо до уничтожения канала. Один канал — FIFO-очередь, а не пара концов соединения; для независимого обмена в двух направлениях используются два канала.

[`fvm_worker_stop`](../include/flashvm/worker.h#L52) только запрашивает остановку и не прерывает заблокированный `receive`. Чтобы разбудить такой поток, владелец должен закрыть соответствующий канал перед `join`. Уничтожать канал, пока другой поток им пользуется, нельзя. Автоматическое управление временем жизни общих каналов ещё не реализовано.

Рабочий пример C89 находится в `examples/embedding/worker_channels/`. Синтаксис потоков и каналов ActionScript пока запланирован, но не реализован.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:26](../src/capi/channel_c.c#L26) |
| `fvm_channel_receive` | function | [include/flashvm/channel.h:55](../include/flashvm/channel.h#L55) |
| `fvm_channel_send` | function | [include/flashvm/channel.h:53](../include/flashvm/channel.h#L53) |
| `fvm_channel_try_receive` | function | [include/flashvm/channel.h:70](../include/flashvm/channel.h#L70) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_Worker` | struct | [src/capi/worker_c.c:12](../src/capi/worker_c.c#L12) |
| `fvm_worker_stop` | function | [include/flashvm/worker.h:52](../include/flashvm/worker.h#L52) |
<!-- source-links:end -->
