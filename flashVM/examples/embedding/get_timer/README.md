# Context clock via getTimer

Executable C89 example: install the standard library and read the AS1/AS2
getTimer global without a SWF, device, network, worker or delay.
[Русский](README.ru.md). [Clock contract](../../../doc/GET_TIMER.md).

From the FlashVM component:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_get_timer_main
./build/flashvm_examples_embedding_get_timer_main
```

Expected output: `context elapsed milliseconds: N`, where N is a nonnegative
integer below 2^32. Its exact value depends on platform timing and installation
duration. A clock error is reported with a nonzero exit code.

## APIs, ownership and blocking

| Function | Demonstrated contract |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L70) | Initialize caller-owned ABI defaults before acquisition. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L74) | Copy configuration and acquire the outer runtime owner. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L82) | Acquire the owning-thread heap and its independent clock origin. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L22) | Publish getTimer and standard objects; report conflicting own bindings without replacing them. |
| [fvm_call](../../../include/flashvm/value.h#L204) | Synchronously read getTimer with zero arguments; returns Number or a clock/allocation error. No waiting or callbacks. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L86) | Release the heap and standard bindings before destroying their runtime. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L78) | Release the outer owner after all its contexts. |

The runtime outlives the context; the plain numeric result owns no buffer.
All calls run on the owner thread. Setup and invocation may allocate; the clock
callback itself performs no allocation, coercion, waiting or scheduling.
The origin is context creation, with unsigned modulo-2^32 wrapping, rather than
accepted original-player movie-start/signed-wrap semantics. This example creates
no interval or timeout. See the linked clock contract for recovery and limits.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:204](../../../include/flashvm/value.h#L204) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../../../include/flashvm/standard.h#L22) |
<!-- source-links:end -->
