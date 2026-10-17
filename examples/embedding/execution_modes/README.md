# embedding/execution_modes

Задаёт режимы исполнения на входе и во время работы, затем демонстрирует opt-in replay ошибки в интерпретаторе. Показывает счётчики и диагностические callback; повторное исполнение требует учёта побочных эффектов.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal-jit.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_execution_modes_main --parallel
./build-examples/flashvm_examples_embedding_execution_modes_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_execution_modes_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_exception_clear`](../../../include/flashvm/exception.h) | [STANDARD_ERROR.md](../../../doc/STANDARD_ERROR.md) — Ошибки и сохранение исходного исключения. |
| [`fvm_exception_get`](../../../include/flashvm/exception.h) | [STANDARD_ERROR.md](../../../doc/STANDARD_ERROR.md) — Ошибки и сохранение исходного исключения. |
| [`fvm_exception_raise`](../../../include/flashvm/exception.h) | [STANDARD_ERROR.md](../../../doc/STANDARD_ERROR.md) — Ошибки и сохранение исходного исключения. |
| [`fvm_execution_get_stats`](../../../include/flashvm/execution_mode.h) | [DEBUG_API.md#fvm_execution_get_stats](../../../doc/DEBUG_API.md#fvm_execution_get_stats) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_execution_options_init`](../../../include/flashvm/execution_mode.h) | [DEBUG_API.md#fvm_execution_options_init](../../../doc/DEBUG_API.md#fvm_execution_options_init) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_execution_set_mode`](../../../include/flashvm/execution_mode.h) | [DEBUG_API.md#fvm_execution_set_mode](../../../doc/DEBUG_API.md#fvm_execution_set_mode) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_function_create_avm1`](../../../include/flashvm/function.h) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_function_invoke_options`](../../../include/flashvm/execution_mode.h) | [DEBUG_API.md#fvm_function_invoke_options](../../../doc/DEBUG_API.md#fvm_function_invoke_options) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_function_set_body`](../../../include/flashvm/function.h) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_jit_available`](../../../include/flashvm/jit.h) | [DEBUG_API.md#fvm_jit_available](../../../doc/DEBUG_API.md#fvm_jit_available) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_jit_backend_name`](../../../include/flashvm/jit.h) | [DEBUG_API.md#fvm_jit_backend_name](../../../doc/DEBUG_API.md#fvm_jit_backend_name) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_object_create`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_standard_install`](../../../include/flashvm/standard.h) | [STANDARD_OBJECTS.md](../../../doc/STANDARD_OBJECTS.md) — Стандартные классы и объектная модель. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-execution_modes).
<!-- example-catalog-api:end -->
