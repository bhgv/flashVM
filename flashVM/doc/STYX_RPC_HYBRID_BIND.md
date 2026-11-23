# fvm_styxrpc_hybrid_bind

[Русский](STYX_RPC_HYBRID_BIND.ru.md) · [Declaration](../include/flashvm/styxrpc_hybrid.h) · [Implementation](../src/styxrpc/hybrid.c) · [Table methods](STYX_RPC_HYBRID.md)

Include `flashvm/styxrpc_hybrid.h` and link the optional `flashvm_styxrpc` target.

```c
FVM_Result fvm_styxrpc_hybrid_bind(FVM_Context *context, const char *global);
```

On the owner of `context`, register the nonempty `global` name for a callable constructor. The name is consumed during registration. Each zero-argument call creates a new empty table. The constructor does not use a session or wait on a remote peer. The returned object and all methods belong to the calling context. Root a retained table as with any FlashVM object.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) means the function was registered. A missing context or empty name yields [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). The normal native registration errors propagate. A constructor call with arguments returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47), and allocation failure reports [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46). Tables decoded from a graph already have their methods, so they can be used without separately registering the constructor.

[Executable AVM1 fixture](../tests/styxrpc/hybrid.c)

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
| `fvm_styxrpc_hybrid_bind` | function | [include/flashvm/styxrpc_hybrid.h:16](../include/flashvm/styxrpc_hybrid.h#L16) |
<!-- source-links:end -->
