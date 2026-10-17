# embedding/jit_debug

Добавляет ручную source map и breakpoint, затем читает статистику исполнения. Демонстрирует исключение выбранного тела из JIT на следующем входе и синхронную остановку интерпретатора.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal-jit.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_jit_debug_main --parallel
./build-examples/flashvm_examples_embedding_jit_debug_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_jit_debug_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_debug_map_action`](../../../include/flashvm/debug.h) | [DEBUG_API.md#fvm_debug_map_action](../../../doc/DEBUG_API.md#fvm_debug_map_action) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_debug_set_breakpoint`](../../../include/flashvm/debug.h) | [DEBUG_API.md#fvm_debug_set_breakpoint](../../../doc/DEBUG_API.md#fvm_debug_set_breakpoint) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_debug_set_breakpoint_handler`](../../../include/flashvm/debug.h) | [DEBUG_API.md#fvm_debug_set_breakpoint_handler](../../../doc/DEBUG_API.md#fvm_debug_set_breakpoint_handler) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_execution_get_stats`](../../../include/flashvm/execution_mode.h) | [DEBUG_API.md#fvm_execution_get_stats](../../../doc/DEBUG_API.md#fvm_execution_get_stats) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_execution_set_mode`](../../../include/flashvm/execution_mode.h) | [DEBUG_API.md#fvm_execution_set_mode](../../../doc/DEBUG_API.md#fvm_execution_set_mode) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_function_create_avm1`](../../../include/flashvm/function.h) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_function_invoke`](../../../include/flashvm/function.h) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_jit_backend_name`](../../../include/flashvm/jit.h) | [DEBUG_API.md#fvm_jit_backend_name](../../../doc/DEBUG_API.md#fvm_jit_backend_name) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-jit_debug).
<!-- example-catalog-api:end -->
