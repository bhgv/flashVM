# Extending FlashVM from C89

`main.c` registers the `makePoint` native callback with the current C API.
The callback validates two numeric arguments, creates a VM object,
sets its `x` and `y` properties and returns it to the C host.
Comments document the ownership and registration behavior.

Expected output:

    makePoint => (12, 34)

Build from the repository root:

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

Executable: `flashvm_examples_extending_main` (when `FVM_BUILD_EXAMPLES=ON`).
This is a **statically registered** extension, not a loadable `.so` / `.dll`.
This tutorial does not implement a dynamic module loader; it calls the registered callback from C.

Read [stack API](../../doc/STACK_API.md) and [AVM1 functions](../../doc/AVM1_FUNCTIONS.md).

<!-- example-catalog-api:start -->
## Назначение и особенности

Регистрирует makePoint, проверяет два числовых аргумента, создаёт объект x/y и возвращает его C-хосту. Это статическая регистрация native callback, а не загрузчик динамической библиотеки.

**Текущая CMake-цель:** `flashvm_examples_extending_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_call`](../../include/flashvm/value.h) | [API_INDEX.md#native-calls-and-metatables](../../doc/API_INDEX.md#native-calls-and-metatables) — Вызов именованной функции. |
| [`fvm_config_init`](../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_object_create`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_get`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_register_function`](../../include/flashvm/value.h) | [API_INDEX.md#native-calls-and-metatables](../../doc/API_INDEX.md#native-calls-and-metatables) — Регистрация именованного native callback. |
| [`fvm_runtime_create`](../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_number`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../README.md#example-extending-main).
<!-- example-catalog-api:end -->
