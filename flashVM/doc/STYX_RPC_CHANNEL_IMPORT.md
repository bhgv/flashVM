# fvm_styxrpc_channel_import

[Русский](STYX_RPC_CHANNEL_IMPORT.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h) · [Implementation](../src/styxrpc/channel_import.c) · [Channel methods](STYX_RPC_CHANNELS.md)

Include `flashvm/styxrpc_channel.h` and link `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_import(FVM_Context *context, const char *global,
    StyxRpcManagedTarget *target);
```

On the FlashVM owner `context`, install a callable AS global named `global`. Supply a mounted `target`, exact module and export names, compiled signature, complete schema, delegated nonzero PID and polling budget in milliseconds. Keep the descriptor and its strings alive while the function is callable, and keep the session active during each call. Changes to the descriptor between calls select a new target. Existing channel objects hold their original leases and copied operation budgets.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) reports registration. Missing or invalid input returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Other errors from native registration propagate. A call checks the target profile, exchanges one frame and turns returned live channels into owner-local objects. A call that fails after submission can still have run remotely. Close the channels explicitly for synchronous retirement, or let deferred GC cleanup release them. Shutdown of the borrowed session invalidates its outstanding operations.

[Executable fixture](../tests/styxrpc/channel.c)

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
| `fvm_styxrpc_channel_import` | function | [include/flashvm/styxrpc_channel.h:20](../include/flashvm/styxrpc_channel.h#L20) |
<!-- source-links:end -->
