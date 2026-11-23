# fvm_styxrpc_channel_handler_export

[English](STYX_RPC_CHANNEL_HANDLER_EXPORT.md) · [Объявление](../include/flashvm/styxrpc_channel.h) · [Реализация](../src/styxrpc/channel_handler.c) · [Сценарий аргументов](STYX_RPC_CHANNEL_ARGUMENTS.ru.md)

Подключите `<flashvm/styxrpc_channel.h>` и линкуйте `flashvm_styxrpc` в хостовой сборке с `FVM_ENABLE_STYXRPC=ON`.

```c
FVM_Result fvm_styxrpc_channel_handler_export(FVM_StyxRpcChannelHandlerExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported);
```

| Параметр | Контракт |
| --- | --- |
| `binding` | Обязателен. `handler` задаёт контекст владельца, имя функции, смонтированный встречный клиент, компилированную схему и ожидание одной встречной операции от 1 до 60000 мс. Схема содержит хотя бы один канал-аргумент и ровно один канал результата. `capacity` задаёт от 0 до 8 слотов FIFO. Ноль выбирает rendezvous. |
| `name` | Обязательное компилированное имя экспорта и компонента файла Styx. |
| `signature` | Точная 32-битная сигнатура от компилятора Limbo. |
| `exported` | Описатель в памяти вызывающего, заполняемый при успехе. Передайте его [модулю SDK с владельцем колбэков](../../lib/styxrpc/reference/channel_handlers_create_with_results.ru.md). |

Возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44) с заполненным описателем или [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47), сохраняя выходные данные. В сообщениях каналов-аргументов допустимы [каналы той же шины](STYX_RPC_NESTED_CHANNELS.ru.md). Выданный канал результата передаёт элементы по значению. Возвращаемое колбэком значение становится первым элементом нового FIFO. `undefined` создаёт пустой FIFO или rendezvous. Исключение сценария, неподходящий элемент либо ошибка публикации дают ошибку RPC.

Выполняйте колбэк на владельце FlashVM. Прямой сервер Styx работает отдельно. Сохраняйте привязку, имена, контекст и встречный клиент одной сессии до остановки обоих потоков. Объекты аргументов закрываются при выходе из колбэка. Результат получает собственный ресурс и аренду. Диспетчер публикует его через `.unibus` прямого модуля и отзывает при clunk выдавшего fid функции.

[Рабочий хост FlashVM](../tests/styxrpc/channel-incoming.c) · [Вызывающий код Limbo](../../lib/styxrpc/tests/flashvm/channel_incoming.b) · [Колбэк с копируемым результатом](STYX_RPC_CHANNEL_HANDLER.ru.md)

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
| `fvm_styxrpc_channel_handler_export` | function | [include/flashvm/styxrpc_channel.h:84](../include/flashvm/styxrpc_channel.h#L84) |
| `FVM_StyxRpcChannelHandlerExport` | struct | [include/flashvm/styxrpc_channel.h:73](../include/flashvm/styxrpc_channel.h#L73) |
<!-- source-links:end -->
