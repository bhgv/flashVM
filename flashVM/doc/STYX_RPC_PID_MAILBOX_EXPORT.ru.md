# fvm_styxrpc_pid_mailbox_export

[English](STYX_RPC_PID_MAILBOX_EXPORT.md) · [Объявление](../include/flashvm/styxrpc_channel.h#L159) · [Реализация](../src/styxrpc/channel_pid_mailbox.c#L203)

```c
#include <flashvm/styxrpc_channel.h>
FVM_Result fvm_styxrpc_pid_mailbox_export(FVM_StyxRpcPidMailboxExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported);
```

Линкуйте `flashvm_styxrpc`.

Вызывайте в потоке-владельце FlashVM, чтобы заполнить `exported` описателем функции с именем `name` и точной сигнатурой `signature`. Храните имя, `binding` и встречное монтирование до завершения модуля. В привязке задаются контекст, инициализатор, компилированные схемы, от 1 до 8 слотов на направление и бюджет встречной операции от 1 до 60000 мс. Вход передаётся по значению, результатом служит один канал с полями-каналами в сообщениях. Например, `(i)` → `(c[c[i]])` выдаёт очередь каналов целых чисел.

Зарегистрируйте описатель в [модуле с арендованными результатами](../../lib/styxrpc/reference/channel_handlers_create_with_results.ru.md). При вызове инициализатор AS получает новый объект очереди, затем декодированные аргументы. Сохраните очередь для работы после возврата. Значение, возвращённое инициализатором, игнорируется. После успеха модуль выдаёт токен результата. Ошибка инициализатора даёт удалённую ошибку вызова.

У владельца `tryReceive()` возвращает undefined при пустой очереди. Для полученного кадра метод проверяет идентификаторы PID на заимствованном монтировании и создаёт живые каналы. Операции этих каналов используют штатную встречную шину. `trySend(value)` возвращает клиенту исходные полномочия PID, уже принятые этой очередью, либо false при полном FIFO. Встречная шина поддерживает [элементы с вложенными каналами](../../lib/styxrpc/reference/channel_pid_exchange.ru.md). Вложенный канал можно отправить в совместимом элементе на шине его PID. Сохранённый вложенный канал остаётся доступным после закрытия родительского прокси. Clunk fid результата закрывает очередь и её прокси для последующих операций. Уже начатые операции могут завершиться. Для освобождения объекта уберите ссылки на него. Храните встречное монтирование, пока возможны операции.

Регистрация возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44) или [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) при неверной привязке, метаданных сигнатуры или неподдерживаемой схеме. [Нативная проверка](../tests/styxrpc/channel-pid-result.c)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_pid_mailbox_export` | function | [include/flashvm/styxrpc_channel.h:159](../include/flashvm/styxrpc_channel.h#L159) |
| `FVM_StyxRpcPidMailboxExport` | struct | [include/flashvm/styxrpc_channel.h:146](../include/flashvm/styxrpc_channel.h#L146) |
<!-- source-links:end -->
