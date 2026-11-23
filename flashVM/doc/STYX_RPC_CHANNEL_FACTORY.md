# fvm_styxrpc_channel_factory

[Русский](STYX_RPC_CHANNEL_FACTORY.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h) · [Implementation](../src/styxrpc/channel_local.c) · [Argument guide](STYX_RPC_CHANNEL_ARGUMENTS.md)

Include `flashvm/styxrpc_channel.h` and link `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_factory(FVM_Context *context, const char *global);
```

Register the nonempty name `global` on the FlashVM context owner. [`FVM_OK`](../include/flashvm/flashvm.h#L44) reports registration. A missing context or empty name returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Native registration errors propagate. The constructor takes exactly two arguments, a compact by-value element type such as `"i"` for int or `"s"` for string and an exact slot count of 0–8. It returns an owned FlashVM object on success. Bad constructor arguments return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) and allocation failure returns [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46).

`trySend(value)` returns true on a local enqueue and false on a full queue. `tryReceive()` consumes one typed element or returns `undefined` when empty. Type violations produce script errors. A zero-capacity queue is locally idle and can rendezvous between Dis operations during an RPC. Calling `close()` repeatedly is harmless and disarms every alias. The object belongs to one owner and GC frees its storage. Use [the argument binding](STYX_RPC_CHANNEL_ARGUMENTS_IMPORT.md) to lend the queue. Resume local access after that call returns.

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
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_channel_factory` | function | [include/flashvm/styxrpc_channel.h:38](../include/flashvm/styxrpc_channel.h#L38) |
<!-- source-links:end -->
