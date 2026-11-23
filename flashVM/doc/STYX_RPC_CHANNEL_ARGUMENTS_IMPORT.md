# fvm_styxrpc_channel_import_arguments

[Русский](STYX_RPC_CHANNEL_ARGUMENTS_IMPORT.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h) · [Implementation](../src/styxrpc/channel_import.c) · [Argument guide](STYX_RPC_CHANNEL_ARGUMENTS.md)

Include `flashvm/styxrpc_channel.h` and link `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_import_arguments(FVM_Context *context, const char *global,
    FVM_StyxRpcChannelArguments *binding);
```

On the owner `context`, register the nonempty callable name `global`. `binding` borrows the exact [managed target](STYX_RPC_CHANNEL_IMPORT.md) and the reverse bus published for its PID on the same connection. The target's schema must include a channel argument. Keep the binding, strings, session and bus alive while in use. Run the reverse Styx dispatcher separately while the FlashVM owner waits.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) reports registration. Invalid configuration returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Registration and script errors propagate. Pass either a [local channel object](STYX_RPC_CHANNEL_FACTORY.md) of the exact argument type, or `null`. The function copies ordinary values and borrows each distinct queue until the scoped call is retired. The SDK revokes the scope on every exit. Completed queue effects remain visible even when the call raises an exception.

[Executable fixture](../tests/styxrpc/channel-args.c)

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
| `fvm_styxrpc_channel_import_arguments` | function | [include/flashvm/styxrpc_channel.h:75](../include/flashvm/styxrpc_channel.h#L75) |
| `FVM_StyxRpcChannelArguments` | struct | [include/flashvm/styxrpc_channel.h:66](../include/flashvm/styxrpc_channel.h#L66) |
<!-- source-links:end -->
