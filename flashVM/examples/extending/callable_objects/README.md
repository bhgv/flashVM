# Callable objects (C89)

This example creates a native callable bound to one interpreter context, stores it
in an ordinary object property, retrieves it, and invokes it with two numbers.
The callback receives the interpreter context and a Lua-inspired argument stack.
It must clear the argument stack and push at most one result before returning.

Build from the repository root with CMake and enable examples. The `main.c`
source is also exercised by the regression test `flashvm_tests_callable`.

**Limitations:** callable objects are currently native C callbacks only; the
This tutorial invokes the callable through the C API. The context
owns the callback registration; `userdata` lifetime is the host's responsibility.
Do not access one context concurrently from multiple threads.

<!-- example-catalog-api:start -->
## Назначение и особенности

Создаёт native callable, сохраняет его в свойстве объекта и вызывает с двумя числами. Callback работает со стеком аргументов и формирует результат; userdata остаётся ответственностью хоста.

**Текущая CMake-цель:** `flashvm_examples_extending_callable_objects_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_callable_create`](../../../include/flashvm/callable.h#L21) | [CALLABLE_OBJECTS.md](../../../doc/CALLABLE_OBJECTS.md) — Вызываемые native-объекты. |
| [`fvm_callable_invoke`](../../../include/flashvm/callable.h#L35) | [CALLABLE_OBJECTS.md](../../../doc/CALLABLE_OBJECTS.md) — Вызываемые native-объекты. |
| [`fvm_callable_is`](../../../include/flashvm/callable.h#L31) | [CALLABLE_OBJECTS.md](../../../doc/CALLABLE_OBJECTS.md) — Вызываемые native-объекты. |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_object_create`](../../../include/flashvm/value.h#L84) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_get`](../../../include/flashvm/value.h#L102) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set`](../../../include/flashvm/value.h#L106) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_stack_clear`](../../../include/flashvm/stack.h#L49) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_get`](../../../include/flashvm/stack.h#L35) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_push`](../../../include/flashvm/stack.h#L41) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_size`](../../../include/flashvm/stack.h#L30) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L50) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L46) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-extending-callable_objects).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_callable_create` | function | [include/flashvm/callable.h:21](../../../include/flashvm/callable.h#L21) |
| `fvm_callable_invoke` | function | [include/flashvm/callable.h:35](../../../include/flashvm/callable.h#L35) |
| `fvm_callable_is` | function | [include/flashvm/callable.h:31](../../../include/flashvm/callable.h#L31) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_object_create` | function | [include/flashvm/value.h:84](../../../include/flashvm/value.h#L84) |
| `fvm_object_get` | function | [include/flashvm/value.h:102](../../../include/flashvm/value.h#L102) |
| `fvm_object_set` | function | [include/flashvm/value.h:106](../../../include/flashvm/value.h#L106) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_stack_clear` | function | [include/flashvm/stack.h:49](../../../include/flashvm/stack.h#L49) |
| `fvm_stack_get` | function | [include/flashvm/stack.h:35](../../../include/flashvm/stack.h#L35) |
| `fvm_stack_push` | function | [include/flashvm/stack.h:41](../../../include/flashvm/stack.h#L41) |
| `fvm_stack_size` | function | [include/flashvm/stack.h:30](../../../include/flashvm/stack.h#L30) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
<!-- source-links:end -->
