# fvm_styxrpc_result_channel_factory

[Русский](STYX_RPC_RESULT_CHANNEL_FACTORY.ru.md) · [Guide](STYX_RPC_RESULT_CHANNELS.md) · [Implementation](../src/styxrpc/channel_result_endpoint.c#L293) · [Header](../include/flashvm/styxrpc_channel.h)

## Syntax

```c
#include <flashvm/styxrpc_channel.h>
FVM_Result fvm_styxrpc_result_channel_factory(FVM_Context *context, const char *global);
```

Link the hosted `flashvm_styxrpc` target. Call on the FlashVM owner with a live `context` and the NUL-terminated nonempty `global` function name. The caller lends both parameters for registration. On [`FVM_OK`](../include/flashvm/flashvm.h#L44) the named script constructor has the [`vfs.resultChannel` contract](STYX_RPC_RESULT_CHANNEL.md). An invalid context or name gives [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). The registration can also return the underlying VM binding or allocation error. It performs no network I/O.

```c
FVM_Result status = fvm_styxrpc_result_channel_factory(context, "newResultChannel");
```

Use `newResultChannel("i", 0)` in the attached movie. Return it through [an export](STYX_RPC_CHANNEL_EXPORT.md) with result shape `(c[i])`. Keep the host provider and callback binding alive as required by that export.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:260](../src/core/runtime.hpp#L260) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_result_channel_factory` | function | [include/flashvm/styxrpc_channel.h:166](../include/flashvm/styxrpc_channel.h#L166) |
<!-- source-links:end -->
