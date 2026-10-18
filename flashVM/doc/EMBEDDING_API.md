# Embedding API (implemented prototype)

FlashVM currently embeds a C89-compatible native value/object runtime backed by C++98. **It does not execute ActionScript or play SWF yet.** Include `flashvm/flashvm.h` and `flashvm/value.h`.

## Lifecycle

Call `fvm_config_init(&config)`, then `fvm_runtime_create(&config, &runtime)` and `fvm_context_create(runtime, &context)`. Check each [`FVM_Result`](../include/flashvm/flashvm.h#L40) against [`FVM_OK`](../include/flashvm/flashvm.h#L42). Destroy the context before its runtime with [`fvm_context_destroy`](../include/flashvm/flashvm.h#L70) and [`fvm_runtime_destroy`](../include/flashvm/flashvm.h#L66). `fvm_abi_version()` returns the current ABI version (2). Feature flags are reserved; setting them does not activate unimplemented modules.

## Values, objects, globals

[`FVM_Value`](../include/flashvm/value.h#L24) has a `type` and fields `number`, `boolean`, `string`, `object`. Use [`fvm_value_undefined`](../include/flashvm/value.h#L40), [`fvm_value_number`](../include/flashvm/value.h#L42), [`fvm_value_boolean`](../include/flashvm/value.h#L44), and [`fvm_value_string`](../include/flashvm/value.h#L46) to initialize values. To pass an object, set `type=FVM_VALUE_OBJECT` and `object` to a pointer returned by [`fvm_object_create`](../include/flashvm/value.h#L62) from the **same context**.

[`fvm_value_string`](../include/flashvm/value.h#L46) borrows the input string without copying it. [`fvm_object_set`](../include/flashvm/value.h#L74) and [`fvm_set_global`](../include/flashvm/value.h#L87) copy strings into context-owned memory. [`fvm_object_get`](../include/flashvm/value.h#L72) and [`fvm_get_global`](../include/flashvm/value.h#L85) return borrowed references: never free them or use them after context destruction. A missing property returns [`FVM_OK`](../include/flashvm/flashvm.h#L42) with an undefined value. Objects remain allocated until context destruction (no GC yet).

## Native calls

Register C callbacks with `fvm_register_function(context, name, callback, userdata)`. Invoke registered callbacks with `fvm_call(context, name, argc, argv, &result)`. Callbacks receive `context`, `self` (currently the global object), arguments, output value, and registration `userdata`. [`fvm_call`](../include/flashvm/value.h#L138) does **not** yet call ActionScript functions. A missing registered function returns [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46).

## SWF / AVM1 inspection

`flashvm/swf.h` exposes [`fvm_swf_inspect`](../include/flashvm/swf.h#L28) for inspecting uncompressed FWS files (versions 1–8); `flashvm/avm1.h` exposes [`fvm_avm1_inspect`](../include/flashvm/avm1.h#L34) for validating action record framing. Set each info struct's `struct_size` to `sizeof(info)` before use. Neither operation executes bytecode. [`fvm_load_memory`](../include/flashvm/flashvm.h#L72) and [`fvm_advance`](../include/flashvm/flashvm.h#L74) are not yet a functional player interface.

## Safety

Contexts are **not thread-safe**; serialize access. No sandbox, dynamic extension loader, native class API, renderer, or JIT is implemented. Never run untrusted native extensions. The public ABI is experimental.

Runnable example: [examples/embedding](../examples/embedding/README.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_abi_version` | function | [include/flashvm/flashvm.h:60](../include/flashvm/flashvm.h#L60) |
| `fvm_advance` | function | [include/flashvm/flashvm.h:74](../include/flashvm/flashvm.h#L74) |
| `fvm_avm1_inspect` | function | [include/flashvm/avm1.h:34](../include/flashvm/avm1.h#L34) |
| `fvm_call` | function | [include/flashvm/value.h:138](../include/flashvm/value.h#L138) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../include/flashvm/flashvm.h#L70) |
| `fvm_get_global` | function | [include/flashvm/value.h:85](../include/flashvm/value.h#L85) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `fvm_object_create` | function | [include/flashvm/value.h:62](../include/flashvm/value.h#L62) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../include/flashvm/value.h#L72) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../include/flashvm/value.h#L74) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_register_function` | function | [include/flashvm/value.h:101](../include/flashvm/value.h#L101) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../include/flashvm/flashvm.h#L66) |
| `fvm_set_global` | function | [include/flashvm/value.h:87](../include/flashvm/value.h#L87) |
| `fvm_swf_inspect` | function | [include/flashvm/swf.h:28](../include/flashvm/swf.h#L28) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
| `fvm_value_boolean` | function | [include/flashvm/value.h:44](../include/flashvm/value.h#L44) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../include/flashvm/value.h#L42) |
| `FVM_VALUE_OBJECT` | enum constant | [include/flashvm/value.h:21](../include/flashvm/value.h#L21) |
| `fvm_value_string` | function | [include/flashvm/value.h:46](../include/flashvm/value.h#L46) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../include/flashvm/value.h#L40) |
<!-- source-links:end -->
