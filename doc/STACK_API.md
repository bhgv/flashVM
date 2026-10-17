# Lua-inspired stack extension API (C89 and C++98)

The API is **inspired by Lua**, but is not source-compatible or binary-compatible with Lua. `FVM_Context*` is always passed to callbacks, and the callback receives an explicit `FVM_Stack*` containing its arguments.

## C interface

Include `flashvm/stack.h`. Register a callback with `fvm_stack_register(context, name, callback, userdata)`. Call it using `fvm_stack_call(context, name, argc, argv, result)`.

A callback receives exactly `argc` entries. Indices start at 1, while negative indices count from the end (`-1` is the last element). Zero is invalid. Read using [`fvm_stack_get`](../include/flashvm/stack.h#L25), push using [`fvm_stack_push`](../include/flashvm/stack.h#L29), remove using [`fvm_stack_pop`](../include/flashvm/stack.h#L31), and reset using [`fvm_stack_clear`](../include/flashvm/stack.h#L33). Return [`FVM_OK`](../include/flashvm/flashvm.h#L42) and leave **exactly one** result; leaving no entries returns `undefined`. More than one result is currently an error.

Values referencing objects must belong to the same context. Strings pushed into a stack are **borrowed pointers**, not deep copies; keep their backing storage valid for the duration of the call. Do not retain a stack pointer after the callback returns. Use existing [`fvm_object_get`](../include/flashvm/value.h#L72), [`fvm_object_set`](../include/flashvm/value.h#L74), [`fvm_get_global`](../include/flashvm/value.h#L85), and [`fvm_set_global`](../include/flashvm/value.h#L87) to manipulate script-visible state.

See `examples/extending/stack_c/main.c` for a complete working example.

## C++98 interface

Include `flashvm/stack.hpp` for the RAII `flashvm::Stack` wrapper. It is implemented solely in C++98 and uses the same C ABI. See `examples/embedding/cpp98_stack/main.cpp`.

## Multiple contexts and threading

Each context has its own function registry. Separate contexts can be created under one runtime, but the current runtime and garbage collector do not establish a documented cross-thread synchronization contract. **Do not access one context concurrently**; host applications must coordinate inter-context communication explicitly. Objects cannot be passed directly between contexts.

## Current limitations

This is an additional C callback calling convention. It does not yet expose function values, Lua-style closures, userdata, registry references, multiple return values, coroutine scheduling, or script-callable registration of stack callbacks. Existing [`FVM_NativeFunction`](../include/flashvm/value.h#L37) APIs remain unchanged. It is not a complete Lua-style extension system.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:237](../src/core/runtime.hpp#L237) |
| `fvm_get_global` | function | [include/flashvm/value.h:85](../include/flashvm/value.h#L85) |
| `FVM_NativeFunction` | callback typedef | [include/flashvm/value.h:37](../include/flashvm/value.h#L37) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../include/flashvm/value.h#L72) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../include/flashvm/value.h#L74) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_set_global` | function | [include/flashvm/value.h:87](../include/flashvm/value.h#L87) |
| `FVM_Stack` | struct | [src/capi/stack_c.cpp:16](../src/capi/stack_c.cpp#L16) |
| `fvm_stack_call` | function | [include/flashvm/stack.h:50](../include/flashvm/stack.h#L50) |
| `fvm_stack_clear` | function | [include/flashvm/stack.h:33](../include/flashvm/stack.h#L33) |
| `fvm_stack_get` | function | [include/flashvm/stack.h:25](../include/flashvm/stack.h#L25) |
| `fvm_stack_pop` | function | [include/flashvm/stack.h:31](../include/flashvm/stack.h#L31) |
| `fvm_stack_push` | function | [include/flashvm/stack.h:29](../include/flashvm/stack.h#L29) |
| `fvm_stack_register` | function | [include/flashvm/stack.h:35](../include/flashvm/stack.h#L35) |
<!-- source-links:end -->
