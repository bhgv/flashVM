# AVM1 stack operations (stage 8)

The interpreter now handles PushDuplicate (0x4c), StackSwap (0x4d), Not
(0x12), Equals (0x0e), Less (0x0f), And (0x10) and Or (0x11).

`Equals`, `Less`, `And` and `Or` currently accept numeric operands only.
Other operand combinations return [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48) rather than silently
pretending to implement Flash coercion rules. `Not` accepts the existing
primitive value representations and objects, but is not a complete AVM1
ToBoolean implementation.

The new `tests/avm1_stack.c` builds an action stream without an ActionScript
compiler and verifies the result via the public C API. Build with
`FVM_BUILD_TESTS=ON` and run `ctest --test-dir build --output-on-failure`.

Not yet implemented: DefineFunction/DefineFunction2, CallMethod, lexical
scope, Flash event dispatch, MovieClip drawing and JIT code generation.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
