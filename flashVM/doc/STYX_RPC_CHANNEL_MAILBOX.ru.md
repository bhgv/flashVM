# fvm_styxrpc_channel_mailbox

[English](STYX_RPC_CHANNEL_MAILBOX.md) · [Объявление](../include/flashvm/styxrpc_channel.h#L135) · [Реализация](../src/styxrpc/channel_mailbox.c#L202)

```c
#include <flashvm/styxrpc_channel.h>
FVM_Result fvm_styxrpc_channel_mailbox(FVM_Context *context, const FVM_Value *anchor,
    const char *shape, unsigned slots, StyxRpcServer *reverse, FVM_Value *out);
```

Линкуйте `flashvm_styxrpc`.

Вызывайте в потоке-владельце `context`. Передайте открытый канал результата в `anchor`, а сервер программ на встречной половине того же подключения Styx в `reverse`. `shape` задаёт схему одного элемента с полями-каналами и завершающим NUL, например `(c[i])`. `slots` задаёт от 1 до 8 мест в каждом направлении. Результат в `out` служит очередью ActionScript и полномочием PID. Присвойте или закрепите его до следующего управляемого выделения.

Отправьте очередь как значение через совместимый канал того же семейства результата. `tryReceive()` возвращает undefined при пустом FIFO. Для извлечённого кадра он проверяет токены на удерживаемом модуле и выбрасывает исключение при отказе. Подтверждение токенов по Styx использует таймаут монтирования. `trySend(value)` принимает живые каналы модуля того же семейства и возвращает false при заполнении исходящего FIFO. `close()` отзывает PID, отменяет ожидающие fid и удаляет непрочитанные кадры. Повторное закрытие допустимо. Сборка мусора также отзывает очередь. Сохраняйте объект достижимым, пока партнёр использует PID. Очередь удерживает аренду и после закрытия исходной обёртки.

Если контекст выполняет отменяемую задачу, остановка прерывает [проверку токена](../../lib/styxrpc/reference/channel_lease_import_message_until.ru.md). Отмена проверки сохраняет ранее допущенные маршруты и сообщает об исключении. Уже извлечённый кадр очереди считается полученным.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) означает успешную публикацию. Неверные аргументы или отказ создания очереди SDK дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Отказ регистрации даёт [`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Возможны ошибки создания объектов и закрепления корней. При отказе `out` сохраняется. Храните встречный сервер до отзыва очереди. Вызывайте вне колбэков встречного диспетчера.

[Руководство](STYX_RPC_CHANNEL_DELEGATION.ru.md) · [Нативная проверка](../tests/styxrpc/channel-mailbox.c)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:260](../src/core/runtime.hpp#L260) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_channel_mailbox` | function | [include/flashvm/styxrpc_channel.h:135](../include/flashvm/styxrpc_channel.h#L135) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
