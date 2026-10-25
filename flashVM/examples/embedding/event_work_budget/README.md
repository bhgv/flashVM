# Bounded native event delivery

Executable strict-C89 tutorial for a headless host that needs bounded event
processing and explicit committed-state handling. It creates a runtime with a
five-unit instruction/work budget, registers three native Mouse observers and
dispatches one primary-pointer event. No keyboard, display, hardware device,
SWF movie or standard script namespace is needed. [Русский](README.ru.md).

Build/run from the FlashVM component:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_event_work_budget_main
./build/flashvm_examples_embedding_event_work_budget_main
```

Expected output:

```text
callbacks=1 pointer=23,45 budget exhausted after publication
```

Independent registrations cost one, three and five units. The event spends
three units on its rooted snapshot, one on publication and one on its first
observer. The next dispatch unit fails before the second observer. The example
expects [`FVM_ERROR`](../../../include/flashvm/flashvm.h#L45), verifies one completed callback and reads the committed
position; it never retries the same event automatically.

## API

| API | Purpose and ownership/blocking |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | Initialize caller-owned configuration before setting the copied runtime work limit. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74) | Acquire a runtime that copies configuration and outlives its contexts. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) | Release the runtime after every associated context; no callback may retain it. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82) | Acquire an independent owning-thread heap. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86) | Release the heap, registration roots and method bindings before borrowed userdata expires. |
| [`fvm_object_create`](../../../include/flashvm/value.h#L84) | Return a managed receiver; its native pointer alone is not a GC root. |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L60) | Acquire a temporary construction root before registration; no script callback. |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L70) | Release only the acquired temporary pin; the successful registration retains the receiver. |
| [`fvm_object_define_method_rooted`](../../../include/flashvm/value.h#L171) | Bind a synchronous native method borrowing the stack counter until context destruction; it does not collect or block. |
| [`fvm_input_add_mouse_listener`](../../../include/flashvm/mouse.h#L26) | Retain receiver identity once; may allocate or fail before publication, never waits for an input device. |
| [`fvm_input_dispatch_mouse`](../../../include/flashvm/mouse.h#L37) | Synchronously publish coordinates and call rooted observers within the shared work budget; committed effects survive later errors. |
| [`fvm_input_get_pointer`](../../../include/flashvm/input.h#L96) | Copy the latest coordinates; no allocation or callbacks. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L46) | Initialize the ignored callback result without acquiring managed ownership. |

[Accounting and errors](../../../doc/INPUT_EVENT_BUDGET.md),
[Mouse contract](../../../doc/MOUSE.md), [GC ownership](../../../doc/GC_NATIVE_ROOTS.md).
The annotated [main.c](main.c) explains every nonempty source line. Cleanup runs
on all failure paths and releases the context before its runtime and borrowed
counter lifetime. Numeric work units are not wall-clock deadlines. Real device,
other-platform, full Flash semantics and arbitrary blocking callback limits are
outside this tutorial; worker cancellation is tested separately.

Private bounds: [tutorial_limits.h](tutorial_limits.h).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../../../include/flashvm/flashvm.h#L45) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:60](../../../include/flashvm/gc.h#L60) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:70](../../../include/flashvm/gc.h#L70) |
| `fvm_input_add_mouse_listener` | function | [include/flashvm/mouse.h:26](../../../include/flashvm/mouse.h#L26) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:37](../../../include/flashvm/mouse.h#L37) |
| `fvm_input_get_pointer` | function | [include/flashvm/input.h:96](../../../include/flashvm/input.h#L96) |
| `fvm_object_create` | function | [include/flashvm/value.h:84](../../../include/flashvm/value.h#L84) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:171](../../../include/flashvm/value.h#L171) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
<!-- source-links:end -->
