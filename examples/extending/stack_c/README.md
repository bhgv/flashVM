# extending/stack_c

Регистрирует callback со стеком FVM_Stack, читает аргументы, очищает стек и возвращает сумму. Показывает индексацию, размер стека и правило единственного результата.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_extending_stack_c_main --parallel
./build-examples/flashvm_examples_extending_stack_c_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_extending_stack_c_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_stack_call`](../../../include/flashvm/stack.h) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_clear`](../../../include/flashvm/stack.h) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_get`](../../../include/flashvm/stack.h) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_push`](../../../include/flashvm/stack.h) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_register`](../../../include/flashvm/stack.h) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_size`](../../../include/flashvm/stack.h) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-extending-stack_c).
<!-- example-catalog-api:end -->
