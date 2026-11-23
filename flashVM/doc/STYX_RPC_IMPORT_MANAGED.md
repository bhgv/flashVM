# fvm_styxrpc_import_managed

[Русский](STYX_RPC_IMPORT_MANAGED.ru.md) · [Header](../include/flashvm/styxrpc_managed.h) · [Implementation](../src/styxrpc/managed.c) · [Bridge](STYX_RPC.md)

Include `flashvm/styxrpc_managed.h` and link `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_import_managed(FVM_Context *context, const char *global,
    StyxRpcManagedTarget *target);
```

Register a callable AS global in the dedicated `context` owner. In `target`,
supply the mounted session, relative module path, export name, compiled
signature, complete schema and reply polling budget. For a scoped export,
publish `target->pid` in the reverse Styx tree through every call. See
[managed invoke](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/reference/managed_invoke.md)
for the transport fields and statuses.

Supported AS mappings are counted Unicode strings, binary64 real, exact `int`, `byte`
and `big` within ±(2⁵³−1), dense sequences, tuples as Arrays, `null` for nil
strings/arrays and `undefined` for a void result. Sequences use a numeric
length and own indexed properties. Reading those properties can execute
getters on the owner thread. A sequence has at most 2048 entries and the
encoded frame has at most 64 KiB. Tuples must match in arity. Arguments and
results are copied by value. [Ordinary ref arguments](STYX_RPC_REFS.md) support copy-back. Reference
graphs, hybrid tables and channels are outside this AS binding.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) means the local registration succeeded. Invalid binding data
returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Each call reports argument, profile,
remote or uncertain failures as script exceptions. A result conversion
failure occurs after the remote call and must not be used as a retry
signal. The call is synchronous, and must run on an owner that can wait.
Keep the target and its strings alive while the function can be called.
Between calls, the owner may clear the session before closing it.

[Executable fixture](../tests/styxrpc/managed.c) · [Export](STYX_RPC_EXPORT_MANAGED.md)

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
| `fvm_styxrpc_import_managed` | function | [include/flashvm/styxrpc_managed.h:26](../include/flashvm/styxrpc_managed.h#L26) |
<!-- source-links:end -->
