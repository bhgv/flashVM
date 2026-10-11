# Callable objects — C API

Header: `flashvm/callable.h` (C89-compatible). Implementation: `src/capi/stack_c.cpp` (C++98).

- `fvm_callable_create(ctx, callback, userdata, &object)` creates a normal
  context-owned object with a native callback binding. The callback has the
  `FVM_StackFunction` signature `(context, stack, userdata)`.
- `fvm_callable_is(ctx, object, &flag)` checks whether the object has a binding.
- `fvm_callable_invoke(ctx, object, argc, argv, &result)` creates a temporary
  argument stack and invokes the callback. Zero results means `undefined`;
  exactly one result is accepted. More than one result is an error.

Use `FVM_Value` with `FVM_VALUE_OBJECT` to store the callable in an ordinary
object property. This is **not** a new `FVM_ValueType` and does not yet support
AVM1 script closures, `__call`, or bytecode-level calls through properties.

Ownership: the context owns objects and callback registrations. The application
owns `userdata` and must keep it alive until callbacks are no longer possible.
Cross-context objects are rejected. Contexts are not thread-safe. An exception
thrown by C++ code in a callback is translated to `FVM_ERROR`.

See `examples/extending/callable_objects/` for a C89 example.
