# fvm_styxrpc_channel_export

[English](STYX_RPC_CHANNEL_EXPORT.md) · [Объявление](../include/flashvm/styxrpc_channel.h) · [Реализация](../src/styxrpc/channel_export.c) · [Описание каналов](STYX_RPC_CHANNELS.ru.md)

Подключите `flashvm/styxrpc_channel.h` и цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_export(FVM_StyxRpcChannelExport *binding,
    const char *name, uint32_t signature, StyxRpcChannelExport *exported);
```

В `binding` задайте владельца FlashVM, имя инициализатора AS, полную схему и ёмкость. Схема содержит аргументы по значению и один типизированный канал в результате. Ёмкость составляет 0–8 элементов. Значение результата инициализатора помещается в первый буферный слот. Результат `undefined` оставляет канал пустым и обязателен при нулевой ёмкости.

Передайте компилированное имя `name`, подпись `signature` и память `exported`. [`FVM_OK`](../include/flashvm/flashvm.h#L44) подготавливает дескриптор для [публикации модуля каналов](../../lib/styxrpc/reference/channel_module_init.ru.md). Неверные аргументы дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Исключение AS, неверное начальное значение или нехватка памяти при вызове приводят к удалённой ошибке. Инициализатор выполняется синхронно на владельце FlashVM. Сохраняйте привязку и имена до уничтожения сервера. Каждый успешный fid функции владеет группой каналов и её отменой. Элемент результата может описывать [вложенные каналы](STYX_RPC_NESTED_CHANNELS.ru.md) на той же аренде.

[Исполняемая проверка AVM1](../tests/styxrpc/channel-service.c)

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
| `fvm_styxrpc_channel_export` | function | [include/flashvm/styxrpc_channel.h:69](../include/flashvm/styxrpc_channel.h#L69) |
| `FVM_StyxRpcChannelExport` | struct | [include/flashvm/styxrpc_channel.h:58](../include/flashvm/styxrpc_channel.h#L58) |
<!-- source-links:end -->
