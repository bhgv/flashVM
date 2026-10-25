> Historical checkpoint. Актуальные изменения v0.13: [EN](STANDARD_OBJECTS.md), [RU](STANDARD_OBJECTS.ru.md).

# AVM1 script functions (experimental)

This milestone adds a deliberately limited execution path for named AVM1 `DefineFunction` (0x9B), `CallFunction` (0x3D), and `Return` (0x3E).

Supported: a named function with zero parameters, a bytecode body, a return value, and calls from the same context. Native C functions remain callable through the existing registration API. Functions are registered in the context and are available to subsequent [`fvm_avm1_execute`](../include/flashvm/avm1.h#L46) calls.

Not supported: arguments to script functions, anonymous functions, lexical scopes, closures, `DefineFunction2`, function objects, `CallMethod`, complete AVM1 coercion semantics, and proper shared instruction-budget accounting across nested calls. The 64-level call-depth guard is not a substitute for a total execution budget. Do not run untrusted SWF files with this experimental implementation.

The `tests/avm1_functions.c` fixture checks that a zero-argument function can return a value to its caller. The example ActionScript source in `examples/embedding/script_functions` illustrates the intended bytecode sequence; it is not a complete SWF compiler test.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_avm1_execute` | function | [include/flashvm/avm1.h:46](../include/flashvm/avm1.h#L46) |
<!-- source-links:end -->
