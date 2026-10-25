# Callable objects — C API

Header: `flashvm/callable.h` (C89-compatible). Implementation: `src/capi/stack_c.cpp` (C++98).

- `fvm_callable_create(ctx, callback, userdata, &object)` creates a normal
  context-owned object with a native callback binding. The callback has the
  [`FVM_StackFunction`](../include/flashvm/stack.h#L18) signature `(context, stack, userdata)`.
- `fvm_callable_is(ctx, object, &flag)` checks whether the object has a binding.
- `fvm_callable_invoke(ctx, object, argc, argv, &result)` creates a temporary
  argument stack and invokes the callback. Zero results means `undefined`;
  exactly one result is accepted. More than one result is an error.

Use [`FVM_Value`](../include/flashvm/value.h#L26) with [`FVM_VALUE_OBJECT`](../include/flashvm/value.h#L21) to store the callable in an ordinary
object property. This is **not** a new [`FVM_ValueType`](../include/flashvm/value.h#L14) and does not yet support
AVM1 script closures, `__call`, or bytecode-level calls through properties.

Ownership: the context owns objects and callback registrations. The application
owns `userdata` and must keep it alive until callbacks are no longer possible.
Cross-context objects are rejected. Contexts are not thread-safe. An exception
thrown by C++ code in a callback is translated to [`FVM_ERROR`](../include/flashvm/flashvm.h#L45).

See `examples/extending/callable_objects/` for a C89 example.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_callable_create` | function | [include/flashvm/callable.h:21](../include/flashvm/callable.h#L21) |
| `fvm_callable_invoke` | function | [include/flashvm/callable.h:35](../include/flashvm/callable.h#L35) |
| `fvm_callable_is` | function | [include/flashvm/callable.h:31](../include/flashvm/callable.h#L31) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_StackFunction` | callback typedef | [include/flashvm/stack.h:18](../include/flashvm/stack.h#L18) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
| `FVM_VALUE_OBJECT` | enum constant | [include/flashvm/value.h:21](../include/flashvm/value.h#L21) |
| `FVM_ValueType` | enum | [include/flashvm/value.h:14](../include/flashvm/value.h#L14) |
<!-- source-links:end -->
