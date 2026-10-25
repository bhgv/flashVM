# Lua-inspired stack extension API (C89 and C++98)

The API is **inspired by Lua**, but is not source-compatible or binary-compatible with Lua. `FVM_Context*` is always passed to callbacks, and the callback receives an explicit `FVM_Stack*` containing its arguments.

## C interface

Include `flashvm/stack.h`. Register a callback with `fvm_stack_register(context, name, callback, userdata)`. Call it using `fvm_stack_call(context, name, argc, argv, result)`.

A callback receives exactly `argc` entries. Indices start at 1, while negative indices count from the end (`-1` is the last element). Zero is invalid. Read using [`fvm_stack_get`](../include/flashvm/stack.h#L35), push using [`fvm_stack_push`](../include/flashvm/stack.h#L41), remove using [`fvm_stack_pop`](../include/flashvm/stack.h#L45), and reset using [`fvm_stack_clear`](../include/flashvm/stack.h#L49). Return [`FVM_OK`](../include/flashvm/flashvm.h#L44) and leave **exactly one** result; leaving no entries returns `undefined`. More than one result is currently an error.

Values referencing objects must belong to the same context. Strings pushed into a stack are **borrowed pointers**, not deep copies; keep their backing storage valid for the duration of the call. Do not retain a stack pointer after the callback returns. Use existing [`fvm_object_get`](../include/flashvm/value.h#L102), [`fvm_object_set`](../include/flashvm/value.h#L106), [`fvm_get_global`](../include/flashvm/value.h#L125), and [`fvm_set_global`](../include/flashvm/value.h#L129) to manipulate script-visible state.

See `examples/extending/stack_c/main.c` for a complete working example.

## C++98 interface

Include `flashvm/stack.hpp` for the RAII `flashvm::Stack` wrapper. It is implemented solely in C++98 and uses the same C ABI. See `examples/embedding/cpp98_stack/main.cpp`.

## Multiple contexts and threading

Each context has its own function registry. Separate contexts can be created under one runtime, but the current runtime and garbage collector do not establish a documented cross-thread synchronization contract. **Do not access one context concurrently**; host applications must coordinate inter-context communication explicitly. Objects cannot be passed directly between contexts.

## Current limitations

This is an additional C callback calling convention. It does not yet expose function values, Lua-style closures, userdata, registry references, multiple return values, coroutine scheduling, or script-callable registration of stack callbacks. Existing [`FVM_NativeFunction`](../include/flashvm/value.h#L41) APIs remain unchanged. It is not a complete Lua-style extension system.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `fvm_get_global` | function | [include/flashvm/value.h:125](../include/flashvm/value.h#L125) |
| `FVM_NativeFunction` | callback typedef | [include/flashvm/value.h:41](../include/flashvm/value.h#L41) |
| `fvm_object_get` | function | [include/flashvm/value.h:102](../include/flashvm/value.h#L102) |
| `fvm_object_set` | function | [include/flashvm/value.h:106](../include/flashvm/value.h#L106) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_set_global` | function | [include/flashvm/value.h:129](../include/flashvm/value.h#L129) |
| `FVM_Stack` | struct | [src/capi/stack_c.cpp:18](../src/capi/stack_c.cpp#L18) |
| `fvm_stack_call` | function | [include/flashvm/stack.h:76](../include/flashvm/stack.h#L76) |
| `fvm_stack_clear` | function | [include/flashvm/stack.h:49](../include/flashvm/stack.h#L49) |
| `fvm_stack_get` | function | [include/flashvm/stack.h:35](../include/flashvm/stack.h#L35) |
| `fvm_stack_pop` | function | [include/flashvm/stack.h:45](../include/flashvm/stack.h#L45) |
| `fvm_stack_push` | function | [include/flashvm/stack.h:41](../include/flashvm/stack.h#L41) |
| `fvm_stack_register` | function | [include/flashvm/stack.h:53](../include/flashvm/stack.h#L53) |
<!-- source-links:end -->
