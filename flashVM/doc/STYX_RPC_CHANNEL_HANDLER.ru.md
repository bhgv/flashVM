# fvm_styxrpc_channel_handler

[English](STYX_RPC_CHANNEL_HANDLER.md) · [Объявление](../include/flashvm/styxrpc_channel.h) · [Реализация](../src/styxrpc/channel_handler.c) · [Сценарий аргументов](STYX_RPC_CHANNEL_ARGUMENTS.ru.md)

Подключите `<flashvm/styxrpc_channel.h>` и линкуйте `flashvm_styxrpc` в хостовой сборке с `FVM_ENABLE_STYXRPC=ON`.

```c
FVM_Result fvm_styxrpc_channel_handler(FVM_StyxRpcChannelHandler *binding,
    const char *name, uint32_t signature, StyxRpcChannelFunction *function);
```

| Параметр | Контракт |
| --- | --- |
| `binding` | Обязателен. `context` задаёт владельца колбэка FlashVM. Поле `function` внутри привязки задаёт имя глобальной вызываемой функции в этом контексте. `reverse` — смонтированный встречный клиент данного соединения. `schema` хранит точные списки типов аргументов и результата. `wait_ms` — время ожидания одной операции в миллисекундах, 1–60000, без значения по умолчанию. |
| `name` | Точное компилированное имя экспорта Limbo и компонента файла Styx. Обязательно. |
| `signature` | 32-битная сигнатура экспорта от компилятора Limbo. |
| `function` | Обязательный выходной описатель в памяти вызывающего для [создания хостового модуля](../../lib/styxrpc/reference/channel_handlers_create.ru.md). |

Возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44) с заполненным описателем или [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47), сохраняя выходные данные. В схеме должен быть хотя бы один аргумент-канал. Копируемые аргументы и результаты используют управляемые типы значений. Элементы каналов-аргументов могут содержать [каналы той же шины](STYX_RPC_NESTED_CHANNELS.ru.md). Допустим результат void. Канал nil декодируется в `null`.

Привязка только заполняет описатель. Выполняйте [шаги колбэков](../../lib/styxrpc/reference/channel_handlers_step.ru.md) на владельце контекста FlashVM. Прямой диспетчер Styx работает отдельно, пока метод канала ждёт. Сохраняйте привязку, имена, контекст и встречный клиент до завершения сервера и активного колбэка. Каждый аргумент-канал закрывается при выходе из колбэка, включая исключение и ошибку кодирования результата. Эффекты завершённых операций видны в DisVM. Ошибки при исполнении колбэка передаются как ошибки RPC.

[Рабочая настройка хоста и колбэк](../tests/styxrpc/channel-incoming.c) · [Вызывающая программа Limbo](../../lib/styxrpc/tests/flashvm/channel_incoming.b)

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
| `fvm_styxrpc_channel_handler` | function | [include/flashvm/styxrpc_channel.h:113](../include/flashvm/styxrpc_channel.h#L113) |
| `FVM_StyxRpcChannelHandler` | struct | [include/flashvm/styxrpc_channel.h:99](../include/flashvm/styxrpc_channel.h#L99) |
<!-- source-links:end -->
