# Missing methods resolved to native C Functions

[Русский](README.ru.md) · [Source](main.c)

This C89 example loads a minimal SWF6, installs `Object.__resolve`, and calls
an absent `sum` method. The resolver returns the original native C Function;
the VM invokes it with the original receiver and arguments. No `sum` property
is cached. Successful output is `sum = 42`.

From the repository root, with CMake and C/C++ compilers supporting C89/C++98:

```sh
cmake -S . -B build-resolve -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-resolve --config Release --target flashvm_examples_embedding_property_resolve_main
./build-resolve/flashvm_examples_embedding_property_resolve_main
```

Windows generators may place the `.exe` in `Release/`. Foreign OS execution
has not been validated by this Linux check.

The receiver is pinned during construction. Temporary Function pins protect
publication; the resolver's managed capture retains the original C Function.
Userdata alone is not a GC root. Native callbacks retain the normal collection
barrier. Context destruction releases the receiver graph, including pins.

This example runs locally. For original C functions served over one broker port
between independent processes, two runtimes on separate threads, or computers
on a LAN, see [function_rpc](../function_rpc/README.md). For hook semantics and
remaining limitations, see [OBJECT_RESOLVE](../../../doc/OBJECT_RESOLVE.md).

<!-- example-catalog-api:start -->
## Назначение и особенности

Подключает ленивое разрешение отсутствующих свойств через __resolve и вызывает его через API/AVM1. Показывает receiver, повторное использование функции, GC-корни и отличие отсутствующего свойства от собственного поля.

**Текущая CMake-цель:** `flashvm_examples_embedding_property_resolve_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_construct`](../../../include/flashvm/value.h#L232) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_function_create_native`](../../../include/flashvm/function.h#L57) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_gc_collect`](../../../include/flashvm/gc.h#L54) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L60) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L70) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_load_memory`](../../../include/flashvm/flashvm.h#L90) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_object_call`](../../../include/flashvm/value.h#L184) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Метод, receiver и аргументы. |
| [`fvm_object_has_own`](../../../include/flashvm/value.h#L98) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set`](../../../include/flashvm/value.h#L106) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L22) | [STANDARD_OBJECTS.md](../../../doc/STANDARD_OBJECTS.md) — Стандартные классы и объектная модель. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L50) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_string_size`](../../../include/flashvm/value.h#L67) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L46) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-property_resolve).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_construct` | function | [include/flashvm/value.h:232](../../../include/flashvm/value.h#L232) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_function_create_native` | function | [include/flashvm/function.h:57](../../../include/flashvm/function.h#L57) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:60](../../../include/flashvm/gc.h#L60) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:70](../../../include/flashvm/gc.h#L70) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:90](../../../include/flashvm/flashvm.h#L90) |
| `fvm_object_call` | function | [include/flashvm/value.h:184](../../../include/flashvm/value.h#L184) |
| `fvm_object_has_own` | function | [include/flashvm/value.h:98](../../../include/flashvm/value.h#L98) |
| `fvm_object_set` | function | [include/flashvm/value.h:106](../../../include/flashvm/value.h#L106) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../../../include/flashvm/standard.h#L22) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
| `fvm_value_string_size` | function | [include/flashvm/value.h:67](../../../include/flashvm/value.h#L67) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
<!-- source-links:end -->
