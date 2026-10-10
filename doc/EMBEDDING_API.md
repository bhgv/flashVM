# Embedding API (implemented prototype)

FlashVM currently embeds a C89-compatible native value/object runtime backed by C++98. **It does not execute ActionScript or play SWF yet.** Include `flashvm/flashvm.h` and `flashvm/value.h`.

## Lifecycle

Call `fvm_config_init(&config)`, then `fvm_runtime_create(&config, &runtime)` and `fvm_context_create(runtime, &context)`. Check each `FVM_Result` against `FVM_OK`. Destroy the context before its runtime with `fvm_context_destroy` and `fvm_runtime_destroy`. `fvm_abi_version()` returns the current ABI version (1). Feature flags are reserved; setting them does not activate unimplemented modules.

## Values, objects, globals

`FVM_Value` has a `type` and fields `number`, `boolean`, `string`, `object`. Use `fvm_value_undefined`, `fvm_value_number`, `fvm_value_boolean`, and `fvm_value_string` to initialize values. To pass an object, set `type=FVM_VALUE_OBJECT` and `object` to a pointer returned by `fvm_object_create` from the **same context**.

`fvm_value_string` borrows the input string without copying it. `fvm_object_set` and `fvm_set_global` copy strings into context-owned memory. `fvm_object_get` and `fvm_get_global` return borrowed references: never free them or use them after context destruction. A missing property returns `FVM_OK` with an undefined value. Objects remain allocated until context destruction (no GC yet).

## Native calls

Register C callbacks with `fvm_register_function(context, name, callback, userdata)`. Invoke registered callbacks with `fvm_call(context, name, argc, argv, &result)`. Callbacks receive `context`, `self` (currently the global object), arguments, output value, and registration `userdata`. `fvm_call` does **not** yet call ActionScript functions. A missing registered function returns `FVM_UNSUPPORTED`.

## SWF / AVM1 inspection

`flashvm/swf.h` exposes `fvm_swf_inspect` for inspecting uncompressed FWS files (versions 1–8); `flashvm/avm1.h` exposes `fvm_avm1_inspect` for validating action record framing. Set each info struct's `struct_size` to `sizeof(info)` before use. Neither operation executes bytecode. `fvm_load_memory` and `fvm_advance` are not yet a functional player interface.

## Safety

Contexts are **not thread-safe**; serialize access. No sandbox, dynamic extension loader, native class API, renderer, or JIT is implemented. Never run untrusted native extensions. The public ABI is experimental.

Runnable example: [examples/embedding](../examples/embedding/README.md).
