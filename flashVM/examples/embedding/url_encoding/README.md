# Counted URL encoding

Executable C89 example of AS1/AS2 `escape`/`unescape` through native global calls.
It roundtrips Chinese UTF-16 units, punctuation and an embedded NUL without a
network connection, display or SWF. [Русский](README.ru.md).

From the FlashVM component:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_url_encoding_main
./build/flashvm_examples_embedding_url_encoding_main
```

Expected output:

```text
encoded: %E4%B8%AD%E6%96%87%20%2B%20%40%00Z
roundtrip units: 8 (including NUL)
```

## APIs, ownership and blocking

| Function | Demonstrated contract |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Initialize caller-owned configuration before creating its runtime. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Acquire the runtime; copies configuration and outlives the context. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Acquire the owning-thread heap for bindings and result strings. |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L19) | Publish standard objects and the two URL functions transactionally; preserves conflicts and reports errors. |
| [`fvm_value_string_utf16`](../../../include/flashvm/value.h#L55) | Copy every supplied unit, including NUL, into a context-owned counted string. |
| [`fvm_call`](../../../include/flashvm/value.h#L138) | Synchronously call `escape` or `unescape` with one rooted argument; conversion can allocate or execute user ToString but opens no network/device. |
| [`fvm_value_get_utf16`](../../../include/flashvm/value.h#L60) | Copy decoded units into caller-owned bounded storage and return their counted length. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Release bindings/strings before any managed pointer is used again; invoked on partial failures too. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Release the outer owner after its context. |

The annotated [main.c](main.c) explains every nonempty line. Local values are used
without intervening collection; in traced-string mode, retained results require
reachable VM storage across later GC. Cleanup runs on every error path. Original
malformed escape recovery/codepages and full Flash/platform acceptance are not
claimed. Detailed coercion, work/GC/error and unsupported-input rules:
[EN](../../../doc/URL_FUNCTIONS.md) / [RU](../../../doc/URL_FUNCTIONS.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:138](../../../include/flashvm/value.h#L138) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:19](../../../include/flashvm/standard.h#L19) |
| `fvm_value_get_utf16` | function | [include/flashvm/value.h:60](../../../include/flashvm/value.h#L60) |
| `fvm_value_string_utf16` | function | [include/flashvm/value.h:55](../../../include/flashvm/value.h#L55) |
<!-- source-links:end -->
