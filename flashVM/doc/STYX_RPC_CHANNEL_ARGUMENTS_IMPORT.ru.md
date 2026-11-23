# fvm_styxrpc_channel_import_arguments

[English](STYX_RPC_CHANNEL_ARGUMENTS_IMPORT.md) · [Объявление](../include/flashvm/styxrpc_channel.h) · [Реализация](../src/styxrpc/channel_import.c) · [Сценарий аргументов](STYX_RPC_CHANNEL_ARGUMENTS.ru.md)

Подключите `flashvm/styxrpc_channel.h` и цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_import_arguments(FVM_Context *context, const char *global,
    FVM_StyxRpcChannelArguments *binding);
```

На владельце `context` зарегистрируйте непустое вызываемое имя `global`. `binding` заимствует точную [цель](STYX_RPC_CHANNEL_IMPORT.ru.md) и встречную шину, опубликованную для её PID на том же соединении. Схема цели должна содержать аргумент-канал. Сохраняйте привязку, строки, сессию и шину во время использования. Встречный Styx обслуживается отдельно во время ожидания владельца FlashVM.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) сообщает об успешной регистрации. Неверная конфигурация даёт [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Ошибки регистрации и сценария передаются вызывающему. В аргументе передайте [локальный объект канала](STYX_RPC_CHANNEL_FACTORY.ru.md) точного типа либо `null`. Обычные значения копируются. Каждая отдельная очередь заимствуется до завершения контекстного вызова. SDK отзывает область при любом исходе. Завершённые действия с очередью сохраняются и при исключении.

[Исполняемая проверка](../tests/styxrpc/channel-args.c)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_channel_import_arguments` | function | [include/flashvm/styxrpc_channel.h:48](../include/flashvm/styxrpc_channel.h#L48) |
| `FVM_StyxRpcChannelArguments` | struct | [include/flashvm/styxrpc_channel.h:39](../include/flashvm/styxrpc_channel.h#L39) |
<!-- source-links:end -->
