# Embedding FlashVM from C89

`main.c` demonstrates creating a runtime and context, creating an object,
setting and reading properties, publishing an object as a global, registering
and invoking a native C function, and destroying the context/runtime.
Comments explain ownership and borrowed string lifetime.

Expected output:

    Hello from C host
    nativeSum(10, 20) = 30

Build from the repository root:

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

Executable: `flashvm_examples_embedding_main` (when `FVM_BUILD_EXAMPLES=ON`).
This main demonstrates native host operations; it does not load a movie.

Read [current API index](../../doc/API_INDEX.md#runtime-and-context).

Function interfaces across processes, independent VM threads and LAN:
[function_rpc/README.md](function_rpc/README.md) /
[Русский](function_rpc/README.ru.md), including build and run commands.

<!-- example-catalog-api:start -->
## Назначение и особенности

Базовая интеграция C89: создание runtime/context, объекта и свойства, публикация глобального значения, регистрация и вызов nativeSum. Показывает исходный жизненный цикл хоста и освобождение контекста раньше runtime.

**Текущая CMake-цель:** `flashvm_examples_embedding_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_call`](../../include/flashvm/value.h) | [API_INDEX.md#native-calls-and-metatables](../../doc/API_INDEX.md#native-calls-and-metatables) — Вызов именованной функции. |
| [`fvm_config_init`](../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_get_global`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_create`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_get`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_register_function`](../../include/flashvm/value.h) | [API_INDEX.md#native-calls-and-metatables](../../doc/API_INDEX.md#native-calls-and-metatables) — Регистрация именованного native callback. |
| [`fvm_runtime_create`](../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_set_global`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_number`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_string`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../README.md#example-embedding-main).
<!-- example-catalog-api:end -->
