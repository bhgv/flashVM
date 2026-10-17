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
| [`fvm_callable_create`](../../../include/flashvm/callable.h) | [CALLABLE_OBJECTS.md](../../../doc/CALLABLE_OBJECTS.md) — Вызываемые native-объекты. |
| [`fvm_callable_invoke`](../../../include/flashvm/callable.h) | [CALLABLE_OBJECTS.md](../../../doc/CALLABLE_OBJECTS.md) — Вызываемые native-объекты. |
| [`fvm_callable_is`](../../../include/flashvm/callable.h) | [CALLABLE_OBJECTS.md](../../../doc/CALLABLE_OBJECTS.md) — Вызываемые native-объекты. |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_object_create`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_get`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_stack_clear`](../../../include/flashvm/stack.h) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_get`](../../../include/flashvm/stack.h) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_push`](../../../include/flashvm/stack.h) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_size`](../../../include/flashvm/stack.h) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-extending-callable_objects).
<!-- example-catalog-api:end -->
