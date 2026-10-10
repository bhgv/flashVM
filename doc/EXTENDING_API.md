# Native extension API (implemented prototype)

The current extension mechanism is **statically registered C callbacks**, not dynamic `.so` / `.dll` plugins. See [examples/extending](../examples/extending/README.md).

## Callback contract

`FVM_NativeFunction` in `flashvm/value.h` receives:

- `FVM_Context *context`: owning VM context.
- `FVM_Object *self`: currently the context global object.
- `unsigned int argc`, `const FVM_Value *argv`: borrowed argument array (may be null if `argc == 0`).
- `FVM_Value *result`: output value, to be initialized by the callback.
- `void *userdata`: host-owned pointer supplied at registration.

Check argument count and types before reading fields. Return `FVM_OK` or an `FVM_Result` error. Do not retain pointers to callback arguments. No C++ exceptions may cross the C ABI.

Register with `fvm_register_function(ctx, "makePoint", callback, userdata)` and invoke from the host with `fvm_call`. Registration is per context. Registering an existing name replaces its callback. There is no unregister API yet; keep userdata valid while callable.

## Object access

Callbacks can use `fvm_object_create`, `fvm_object_set`, `fvm_object_get`, `fvm_set_global`, and `fvm_get_global`. An object can be returned by initializing a result to undefined, setting `result->type = FVM_VALUE_OBJECT` and assigning `result->object`. Object pointers cannot cross contexts. Memory is currently context-owned; no retain/release or GC handles exist.

## Not implemented

ActionScript-to-C dispatch, native classes, constructors, methods, getters/setters, drawing, MovieClip creation, event hooks, dynamic loading, extension unload, and versioned function-table ABI are **planned only**. No native-code sandbox exists.
