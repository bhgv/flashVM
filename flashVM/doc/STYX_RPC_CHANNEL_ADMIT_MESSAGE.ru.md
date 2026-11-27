# fvm_styxrpc_channel_admit_message

[English](STYX_RPC_CHANNEL_ADMIT_MESSAGE.md) · [Объявление](../include/flashvm/styxrpc_channel.h#L62) · [Реализация](../src/styxrpc/channel_delegate.c#L221) · [Руководство](STYX_RPC_CHANNEL_DELEGATION.ru.md)

```c
#include <flashvm/styxrpc_channel.h>
FVM_Result fvm_styxrpc_channel_admit_message(FVM_Context *context, const FVM_Value *anchor,
    const char *shape, const void *bytes, size_t size, FVM_Value *out);
```

Линкуйте `flashvm_styxrpc`. В потоке-владельце `context` передайте открытый канал результата в `anchor`. Хост передаёт сообщение, полученное для этого результата на встречной стороне того же соединения Styx. `shape` задаёт схему одного значения с вложенными каналами и завершающим NUL. `bytes` и `size` описывают полный заимствованный элемент UniBus длиной до `STYXRPC_MANAGED_LIMIT` байт.

SDK [подтверждает каждый токен модуля](../../lib/styxrpc/reference/channel_lease_import_message.ru.md), прежде чем принять сообщение. Для проверки используется удерживаемый модуль, каждая операция Styx использует таймаут монтирования. FlashVM возвращает в `out` типизированное значение, в том числе живые объекты каналов, удерживающие исходную аренду. Повторные ссылки с тем же токеном и ключом используют один живой объект. Присвойте или закрепите результат до следующего управляемого выделения.

Если контекст выполняет отменяемую задачу, остановка прерывает [проверку токена](../../lib/styxrpc/reference/channel_lease_import_message_until.ru.md). Отмена проверки сохраняет ранее допущенные маршруты и сообщает об исключении.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) означает успех. [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) охватывает неверные аргументы и кадры, неподдерживаемые элементы, заполнение таблицы и отказ проверки токена. Ошибки корней и выделения могут передаваться вызывающему. [`FVM_ERROR`](../include/flashvm/flashvm.h#L45) означает, что принятый кадр не удалось преобразовать в значение. При ошибке `out` сохраняется. Ошибка преобразования после допуска может оставить проверенные маршруты в таблице. Вызывайте вне колбэков встречного диспетчера.

[Проверка нативного обмена](../tests/styxrpc/channel-delegated.c) · [Допуск отдельно проверенного полномочия](STYX_RPC_CHANNEL_ADMIT_MODULE.ru.md)

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
| `fvm_styxrpc_channel_admit_message` | function | [include/flashvm/styxrpc_channel.h:62](../include/flashvm/styxrpc_channel.h#L62) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
