# fvm_styxrpc_channel_admit_module

[Русский](STYX_RPC_CHANNEL_ADMIT_MODULE.ru.md) · [Declaration](../include/flashvm/styxrpc_channel.h) · [Implementation](../src/styxrpc/channel_delegate.c) · [Delegation guide](STYX_RPC_CHANNEL_DELEGATION.md)

Include `<flashvm/styxrpc_channel.h>` and link the hosted `flashvm_styxrpc` target.

```c
FVM_Result fvm_styxrpc_channel_admit_module(FVM_Context *context, const FVM_Value *anchor,
    const StyxRpcChannelRoute *route, FVM_Value *out);
```

On the FlashVM owner `context`, pass an open result channel as `anchor`. Supply a `STYXRPC_ROUTE_MODULE` grant that the host already authenticated for that exact lease, including the token, key, element schema and rendezvous flag. The bridge copies the route. It uses the module pinned by the anchor's lease for all subsequent operations. Assign or root the resulting `out` before another managed allocation.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) returns a live AS channel with the usual methods. An equal, live token/key pair returns the same AS object. Invalid input, incompatible schema or rendezvous flag for an existing pair, or full lease table returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). An allocation or root failure leaves `out` unchanged and may leave a checked grant admitted for retry. The call itself performs no network I/O. The next operation authenticates the token on the held module.

The resulting channel retains the original issuing fid while it is live. Use `close()` to release that wrapper and its aliases. The host must associate the incoming verified delegation message with this specific issuing result. [C lease admission](../../lib/styxrpc/reference/channel_lease_admit_module.md) · [Executable fixture](../tests/styxrpc/channel-delegated.c)

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
| `fvm_styxrpc_channel_admit_module` | function | [include/flashvm/styxrpc_channel.h:35](../include/flashvm/styxrpc_channel.h#L35) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
