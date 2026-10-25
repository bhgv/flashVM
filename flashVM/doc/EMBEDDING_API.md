# Embedding API (implemented prototype)

FlashVM currently embeds a C89-compatible native value/object runtime backed by C++98. **It does not execute ActionScript or play SWF yet.** Include `flashvm/flashvm.h` and `flashvm/value.h`.

## Lifecycle

Call `fvm_config_init(&config)`, then `fvm_runtime_create(&config, &runtime)` and `fvm_context_create(runtime, &context)`. Check each [`FVM_Result`](../include/flashvm/flashvm.h#L42) against [`FVM_OK`](../include/flashvm/flashvm.h#L44). Destroy the context before its runtime with [`fvm_context_destroy`](../include/flashvm/flashvm.h#L86) and [`fvm_runtime_destroy`](../include/flashvm/flashvm.h#L78). `fvm_abi_version()` returns the current ABI version (2). Feature flags are reserved; setting them does not activate unimplemented modules.

## Values, objects, globals

[`FVM_Value`](../include/flashvm/value.h#L26) has a `type` and fields `number`, `boolean`, `string`, `object`. Use [`fvm_value_undefined`](../include/flashvm/value.h#L46), [`fvm_value_number`](../include/flashvm/value.h#L50), [`fvm_value_boolean`](../include/flashvm/value.h#L54), and [`fvm_value_string`](../include/flashvm/value.h#L58) to initialize values. To pass an object, set `type=FVM_VALUE_OBJECT` and `object` to a pointer returned by [`fvm_object_create`](../include/flashvm/value.h#L84) from the **same context**.

[`fvm_value_string`](../include/flashvm/value.h#L58) borrows the input string without copying it. [`fvm_object_set`](../include/flashvm/value.h#L106) and [`fvm_set_global`](../include/flashvm/value.h#L129) copy strings into context-owned memory. [`fvm_object_get`](../include/flashvm/value.h#L102) and [`fvm_get_global`](../include/flashvm/value.h#L125) return borrowed references: never free them or use them after context destruction. A missing property returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) with an undefined value. Objects remain allocated until context destruction (no GC yet).

## Native calls

Register C callbacks with `fvm_register_function(context, name, callback, userdata)`. Invoke registered callbacks with `fvm_call(context, name, argc, argv, &result)`. Callbacks receive `context`, `self` (currently the global object), arguments, output value, and registration `userdata`. [`fvm_call`](../include/flashvm/value.h#L204) does **not** yet call ActionScript functions. A missing registered function returns [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48).

## SWF / AVM1 inspection

`flashvm/swf.h` exposes [`fvm_swf_inspect`](../include/flashvm/swf.h#L49) for inspecting uncompressed FWS files (versions 1–8); `flashvm/avm1.h` exposes [`fvm_avm1_inspect`](../include/flashvm/avm1.h#L40) for validating action record framing. Set each info struct's `struct_size` to `sizeof(info)` before use. Neither operation executes bytecode. [`fvm_load_memory`](../include/flashvm/flashvm.h#L90) and [`fvm_advance`](../include/flashvm/flashvm.h#L94) are not yet a functional player interface.

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
| `fvm_abi_version` | function | [include/flashvm/flashvm.h:66](../include/flashvm/flashvm.h#L66) |
| `fvm_advance` | function | [include/flashvm/flashvm.h:94](../include/flashvm/flashvm.h#L94) |
| `fvm_avm1_inspect` | function | [include/flashvm/avm1.h:40](../include/flashvm/avm1.h#L40) |
| `fvm_call` | function | [include/flashvm/value.h:204](../include/flashvm/value.h#L204) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../include/flashvm/flashvm.h#L86) |
| `fvm_get_global` | function | [include/flashvm/value.h:125](../include/flashvm/value.h#L125) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:90](../include/flashvm/flashvm.h#L90) |
| `fvm_object_create` | function | [include/flashvm/value.h:84](../include/flashvm/value.h#L84) |
| `fvm_object_get` | function | [include/flashvm/value.h:102](../include/flashvm/value.h#L102) |
| `fvm_object_set` | function | [include/flashvm/value.h:106](../include/flashvm/value.h#L106) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_register_function` | function | [include/flashvm/value.h:153](../include/flashvm/value.h#L153) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../include/flashvm/flashvm.h#L78) |
| `fvm_set_global` | function | [include/flashvm/value.h:129](../include/flashvm/value.h#L129) |
| `fvm_swf_inspect` | function | [include/flashvm/swf.h:49](../include/flashvm/swf.h#L49) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
| `fvm_value_boolean` | function | [include/flashvm/value.h:54](../include/flashvm/value.h#L54) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../include/flashvm/value.h#L50) |
| `FVM_VALUE_OBJECT` | enum constant | [include/flashvm/value.h:21](../include/flashvm/value.h#L21) |
| `fvm_value_string` | function | [include/flashvm/value.h:58](../include/flashvm/value.h#L58) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:46](../include/flashvm/value.h#L46) |
<!-- source-links:end -->
