# fvm_styxrpc_channel_admit_message

[Русский](STYX_RPC_CHANNEL_ADMIT_MESSAGE.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h#L62) · [Implementation](../src/styxrpc/channel_delegate.c#L214) · [Delegation guide](STYX_RPC_CHANNEL_DELEGATION.md)

```c
#include <flashvm/styxrpc_channel.h>
FVM_Result fvm_styxrpc_channel_admit_message(FVM_Context *context, const FVM_Value *anchor,
    const char *shape, const void *bytes, size_t size, FVM_Value *out);
```

Link `flashvm_styxrpc`. On the owner of `context`, pass an open result channel as `anchor`. The host delivers a message it received for this result on the reverse side of the same Styx connection. `shape` is a NUL-terminated schema for one value containing channel leaves. `bytes` and `size` describe its complete borrowed UniBus element up to `STYXRPC_MANAGED_LIMIT` bytes.

The SDK [checks every module token](../../lib/styxrpc/reference/channel_lease_import_message.md) before admitting the message. It uses the pinned module and the mount's timeout for each Styx operation. FlashVM returns a typed value in `out`, including live channel objects that keep the same issuing lease. Repeated references with the same token and key share a live object. Root or assign the result before the next managed allocation.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) reports success. [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) covers bad parameters, malformed or unsupported elements, a full grant table, and a failed token check. Root and allocation errors may propagate. [`FVM_ERROR`](../include/flashvm/flashvm.h#L45) means the admitted frame could not be materialized. On failure `out` stays unchanged. A materialization failure after admission may leave checked routes installed. Call outside reverse dispatcher callbacks.

[Executable native exchange](../tests/styxrpc/channel-delegated.c) · [Admit one already verified grant](STYX_RPC_CHANNEL_ADMIT_MODULE.md)

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
| `fvm_styxrpc_channel_admit_message` | function | [include/flashvm/styxrpc_channel.h:62](../include/flashvm/styxrpc_channel.h#L62) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
