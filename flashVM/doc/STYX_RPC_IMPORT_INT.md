# fvm_styxrpc_import_int

[Русский](STYX_RPC_IMPORT_INT.ru.md) · [Declaration](../include/flashvm/styxrpc.h) · [Implementation](../src/styxrpc/scalar.c) · [Bridge setup](STYX_RPC.md)

Include `flashvm/styxrpc.h` and link `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_import_int(FVM_Context *context, const char *global,
    FVM_StyxRpcImport *binding);
```

On the owner thread, register the named `global` in `context` with a caller-owned
`binding`. Supply an already mounted session, the module's relative path,
the export name and a reply polling budget of 1–60000 ms. [`FVM_OK`](../include/flashvm/flashvm.h#L44) means that the
import is registered. The remote profile is checked on each invocation.
Missing or invalid local arguments return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47).
Registration allocation errors are returned by FlashVM.

The script function accepts one exact Int32 and returns an exact Int32 as a
number. Invalid arguments, refusal before sending, a framed remote error and an
uncertain outcome become distinct script exceptions. A call is never replayed.
The polling budget excludes the Styx session's own I/O deadlines. Execute from a
dedicated owner thread that can wait. Keep the binding and its strings alive
while a reference to the registered function can still be called. When idle,
the owner may clear `binding->session` before closing it. Further calls then
report that the request was not sent.

[Executable fixture](../tests/styxrpc_host.c) · [Export](STYX_RPC_EXPORT_INT.md)

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
| `fvm_styxrpc_import_int` | function | [include/flashvm/styxrpc.h:38](../include/flashvm/styxrpc.h#L38) |
| `FVM_StyxRpcImport` | struct | [include/flashvm/styxrpc.h:17](../include/flashvm/styxrpc.h#L17) |
<!-- source-links:end -->
