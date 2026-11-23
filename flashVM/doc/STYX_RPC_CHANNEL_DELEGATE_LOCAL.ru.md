# fvm_styxrpc_channel_delegate_local

[English](STYX_RPC_CHANNEL_DELEGATE_LOCAL.md) · [Объявление](../include/flashvm/styxrpc_channel.h#L39) · [Реализация](../src/styxrpc/channel_publication.c#L265)

Подключите `<flashvm/styxrpc_channel.h>` и линкуйте `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_delegate_local(FVM_Context *context, const FVM_Value *anchor,
    const FVM_Value *value, StyxRpcServer *reverse, FVM_Value *out);
```

| Параметр | Контракт |
| --- | --- |
| `context` | Контекст FlashVM. Вызывайте в его потоке-владельце вне колбэков встречного диспетчера. |
| `anchor` | Открытый канал арендованного результата DisVM. |
| `value` | Открытая очередь из [локальной фабрики](STYX_RPC_CHANNEL_FACTORY.ru.md) этого контекста. Допускается FIFO или rendezvous. Элементы могут содержать каналы из того же контекста. |
| `reverse` | [Сервер программ](../../lib/styxrpc/reference/program_server_create.ru.md) на встречной половине того же подключения Styx, что у anchor. Эту связь задаёт хост. |
| `out` | Обязательный выход для непрозрачного значения PID. Закрепите или присвойте результат до следующего управляемого выделения. |

Возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44) при публикации. Граф включает до восьми разных очередей, достижимых через занятые слоты, вместе с корневой. Неверный контекст, закрытый или уже переданный канал, превышение размера графа либо ошибка подготовки кадров возвращают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). При невозможности зарегистрировать PID возвращает [`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Ошибки выделения обёртки и массива графа или удержания корней передаются вызывающему. При ошибке `out` сохраняется, резервирование всех очередей отменяется.

Отправьте непрозрачный результат в подходящем сообщении через канал выдавшего семейства. Все опубликованные очереди находятся в исключительном пользовании встречного диспетчера. Для них недоступны локальные `trySend()`, `tryReceive()`, `close()` и заимствование как аргумента вызова. Храните `reverse`, пока не закрыты или не собраны все обёртки каналов и значения PID семейства. Последнее освобождение отзывает PID, отменяет ожидающие fid и возвращает очереди с результатами завершённых операций. При следующем обращении владелец восстанавливает точные ссылки слотов по трассируемому графу. Изменение реестра может ждать текущий встречный запрос. Отзыв из GC выполняется с нативным состоянием без сетевого ввода-вывода.

[Зарегистрировать функцию AS](STYX_RPC_CHANNEL_PUBLISHER.ru.md) · [Рабочий обмен графом с DisVM](../tests/styxrpc/channel-pid-graph.c)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_channel_delegate_local` | function | [include/flashvm/styxrpc_channel.h:39](../include/flashvm/styxrpc_channel.h#L39) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
