# Native extension API (implemented prototype)

The current extension mechanism is **statically registered C callbacks**, not dynamic `.so` / `.dll` plugins. See [examples/extending](../examples/extending/README.md).

## Callback contract

[`FVM_NativeFunction`](../include/flashvm/value.h#L37) in `flashvm/value.h` receives:

- `FVM_Context *context`: owning VM context.
- `FVM_Object *self`: currently the context global object.
- `unsigned int argc`, `const FVM_Value *argv`: borrowed argument array (may be null if `argc == 0`).
- `FVM_Value *result`: output value, to be initialized by the callback.
- `void *userdata`: host-owned pointer supplied at registration.

Check argument count and types before reading fields. Return [`FVM_OK`](../include/flashvm/flashvm.h#L42) or an [`FVM_Result`](../include/flashvm/flashvm.h#L40) error. Do not retain pointers to callback arguments. No C++ exceptions may cross the C ABI.

Register with `fvm_register_function(ctx, "makePoint", callback, userdata)` and invoke from the host with [`fvm_call`](../include/flashvm/value.h#L138). Registration is per context. Registering an existing name replaces its callback. There is no unregister API yet; keep userdata valid while callable.

## Object access

Callbacks can use [`fvm_object_create`](../include/flashvm/value.h#L62), [`fvm_object_set`](../include/flashvm/value.h#L74), [`fvm_object_get`](../include/flashvm/value.h#L72), [`fvm_set_global`](../include/flashvm/value.h#L87), and [`fvm_get_global`](../include/flashvm/value.h#L85). An object can be returned by initializing a result to undefined, setting `result->type = FVM_VALUE_OBJECT` and assigning `result->object`. Object pointers cannot cross contexts. Memory is currently context-owned; no retain/release or GC handles exist.

## Not implemented

ActionScript-to-C dispatch, native classes, constructors, methods, getters/setters, drawing, MovieClip creation, event hooks, dynamic loading, extension unload, and versioned function-table ABI are **planned only**. No native-code sandbox exists.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:138](../include/flashvm/value.h#L138) |
| `FVM_Context` | struct | [src/core/runtime.hpp:239](../src/core/runtime.hpp#L239) |
| `fvm_get_global` | function | [include/flashvm/value.h:85](../include/flashvm/value.h#L85) |
| `FVM_NativeFunction` | callback typedef | [include/flashvm/value.h:37](../include/flashvm/value.h#L37) |
| `FVM_Object` | struct | [src/core/runtime.hpp:119](../src/core/runtime.hpp#L119) |
| `fvm_object_create` | function | [include/flashvm/value.h:62](../include/flashvm/value.h#L62) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../include/flashvm/value.h#L72) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../include/flashvm/value.h#L74) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_register_function` | function | [include/flashvm/value.h:101](../include/flashvm/value.h#L101) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `fvm_set_global` | function | [include/flashvm/value.h:87](../include/flashvm/value.h#L87) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
| `FVM_VALUE_OBJECT` | enum constant | [include/flashvm/value.h:21](../include/flashvm/value.h#L21) |
<!-- source-links:end -->
