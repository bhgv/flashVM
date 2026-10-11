# Lua-inspired stack extension API (C89 and C++98)

The API is **inspired by Lua**, but is not source-compatible or binary-compatible with Lua. `FVM_Context*` is always passed to callbacks, and the callback receives an explicit `FVM_Stack*` containing its arguments.

## C interface

Include `flashvm/stack.h`. Register a callback with `fvm_stack_register(context, name, callback, userdata)`. Call it using `fvm_stack_call(context, name, argc, argv, result)`.

A callback receives exactly `argc` entries. Indices start at 1, while negative indices count from the end (`-1` is the last element). Zero is invalid. Read using `fvm_stack_get`, push using `fvm_stack_push`, remove using `fvm_stack_pop`, and reset using `fvm_stack_clear`. Return `FVM_OK` and leave **exactly one** result; leaving no entries returns `undefined`. More than one result is currently an error.

Values referencing objects must belong to the same context. Strings pushed into a stack are **borrowed pointers**, not deep copies; keep their backing storage valid for the duration of the call. Do not retain a stack pointer after the callback returns. Use existing `fvm_object_get`, `fvm_object_set`, `fvm_get_global`, and `fvm_set_global` to manipulate script-visible state.

See `examples/extending/stack_c/main.c` for a complete working example.

## C++98 interface

Include `flashvm/stack.hpp` for the RAII `flashvm::Stack` wrapper. It is implemented solely in C++98 and uses the same C ABI. See `examples/embedding/cpp98_stack/main.cpp`.

## Multiple contexts and threading

Each context has its own function registry. Separate contexts can be created under one runtime, but the current runtime and garbage collector do not establish a documented cross-thread synchronization contract. **Do not access one context concurrently**; host applications must coordinate inter-context communication explicitly. Objects cannot be passed directly between contexts.

## Current limitations

This is an additional C callback calling convention. It does not yet expose function values, Lua-style closures, userdata, registry references, multiple return values, coroutine scheduling, or script-callable registration of stack callbacks. Existing `FVM_NativeFunction` APIs remain unchanged. It is not a complete Lua-style extension system.
