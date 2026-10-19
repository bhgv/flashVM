# Wheel input / Колесо прокрутки

[main.c](main.c) is an executable, line-commented C89 host using only the public
VM ABI. It registers an onMouseWheel receiver, delivers +3 and -1.5 scroll lines
at independent world positions, reports the callback's delta/coordinates, then
removes the strong registration and releases the construction pin/context/runtime.
No device, display, gesture, or automatic scrolling is required.

Русский: пример показывает явную доставку колеса. Значение — строки прокрутки,
положительное вверх, отрицательное вниз; дробная часть сохраняется. Позиция
публикуется перед обработчиком. Получатель получает один Number; автоматический
поиск объекта под указателем и scrollTarget не выполняются. Нажатия мыши,
фокус, изменение содержимого и прокрутка интерфейса не синтезируются.

## Build and result / Сборка и результат

From the component directory, using any ordinary VM profile:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_mouse_wheel_main -j2
./build/flashvm_examples_embedding_mouse_wheel_main
```

Expected output:

```text
wheel 1: delta=3.0 at 10,20
wheel 2: delta=-1.5 at 30,40
delivered=2 removed=1
```

Русский: SDK/права/физическое колесо не нужны. Это учебное встраивание, не
полный SWF-плеер или приёмка ОС. Ошибки сообщаются в stderr и кодом выхода 1.

## Ownership, thread and errors / Владение, поток и ошибки

All calls run on one VM owner thread outside collection/release/teardown.
Config/counter are native stack data; methods borrow the counter through context
teardown. Pin the new receiver while adding its method; registration also roots
it strongly. Cleanup removes registration before releasing the pin, then destroys
context/runtime. No callback retains args, out, or a borrowed receiver past return.
The callback checks its payload and propagates the pointer-query error unchanged.
See [full wheel contract](../../../doc/MOUSE.md#wheel-events) and
[русский контракт](../../../doc/MOUSE.ru.md#колесо-прокрутки).

## Individual API / Каждая функция

| API | Parameters, behavior, ownership / Параметры и действие |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | config output: initialize ABI defaults, acquire no VM. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | initialized config, runtime output: acquire outer owner, destroy after context. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | live runtime, context output: acquire independent input/listener state. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L18) | context: explicitly install ordinary Object/Array support before optional Mouse. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_standard_mouse_install`](../../../include/flashvm/mouse.h#L36) | context: install SWF6–8 listener namespace atomically; retain conflicts unchanged; not required for direct native delivery. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_object_create`](../../../include/flashvm/value.h#L62) | context, receiver output: context-managed object; temporarily pin before allocating its method. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48) | context, receiver: retain construction graph until cleanup; [GC:] pair only acquired pins with unpin. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_object_define_method_rooted`](../../../include/flashvm/value.h#L115) | context, receiver, onMouseWheel, receive callback, counter userdata, null capture: receiver-bound native handler, borrow userdata until no delivery can occur. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_input_add_mouse_listener`](../../../include/flashvm/mouse.h#L18) | context, receiver: strong identity registration, duplicate succeeds unchanged; allocation failure adds nothing. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_input_dispatch_mouse_wheel`](../../../include/flashvm/mouse.h#L32) | context, finite signed delta in lines, finite world x/y: snapshot roots, publish position/update drag, invoke onMouseWheel(delta); invalid input preserves state, post-publication errors keep committed state. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_input_get_pointer`](../../../include/flashvm/input.h#L75) | context, distinct writable x/y outputs: copy callback-visible world position without querying OS. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L40) | out: initialize ignored handler result; no managed owner acquired. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_input_remove_mouse_listener`](../../../include/flashvm/mouse.h#L20) | context, receiver, removed output: remove identity without allocation; store 0/1 only on success. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L54) | context, receiver: [GC:] release acquired construction pin after final receiver access. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | context: destroy after synchronous callbacks/registration cleanup; all borrowed objects become invalid. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | runtime: release outer owner after its context. [Embedding](../../../doc/EMBEDDING_API.md), [Mouse](../../../doc/MOUSE.md). |

C printf/fprintf report borrowed scalar data; no image or managed pointer is retained.
The reference's optional scrollTarget is deliberately omitted; this sample sends
only the optional delta argument. The host owns device-unit normalization and
whether to deliver zero/fractional lines. See the API contract for statuses,
GC/lifecycle barriers, mutation/error semantics and reference-platform limitations.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_input_add_mouse_listener` | function | [include/flashvm/mouse.h:18](../../../include/flashvm/mouse.h#L18) |
| `fvm_input_dispatch_mouse_wheel` | function | [include/flashvm/mouse.h:32](../../../include/flashvm/mouse.h#L32) |
| `fvm_input_get_pointer` | function | [include/flashvm/input.h:75](../../../include/flashvm/input.h#L75) |
| `fvm_input_remove_mouse_listener` | function | [include/flashvm/mouse.h:20](../../../include/flashvm/mouse.h#L20) |
| `fvm_object_create` | function | [include/flashvm/value.h:62](../../../include/flashvm/value.h#L62) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:115](../../../include/flashvm/value.h#L115) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:18](../../../include/flashvm/standard.h#L18) |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:36](../../../include/flashvm/mouse.h#L36) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
