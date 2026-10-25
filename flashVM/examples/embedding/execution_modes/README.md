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
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_exception_clear`](../../../include/flashvm/exception.h#L74) | [STANDARD_ERROR.md](../../../doc/STANDARD_ERROR.md) — Ошибки и сохранение исходного исключения. |
| [`fvm_exception_get`](../../../include/flashvm/exception.h#L66) | [STANDARD_ERROR.md](../../../doc/STANDARD_ERROR.md) — Ошибки и сохранение исходного исключения. |
| [`fvm_exception_raise`](../../../include/flashvm/exception.h#L78) | [STANDARD_ERROR.md](../../../doc/STANDARD_ERROR.md) — Ошибки и сохранение исходного исключения. |
| [`fvm_execution_get_stats`](../../../include/flashvm/execution_mode.h#L92) | [DEBUG_API.md#fvm_execution_get_stats](../../../doc/DEBUG_API.md#fvm_execution_get_stats) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_execution_options_init`](../../../include/flashvm/execution_mode.h#L74) | [DEBUG_API.md#fvm_execution_options_init](../../../doc/DEBUG_API.md#fvm_execution_options_init) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_execution_set_mode`](../../../include/flashvm/execution_mode.h#L79) | [DEBUG_API.md#fvm_execution_set_mode](../../../doc/DEBUG_API.md#fvm_execution_set_mode) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_function_create_avm1`](../../../include/flashvm/function.h#L62) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_function_invoke_options`](../../../include/flashvm/execution_mode.h#L101) | [DEBUG_API.md#fvm_function_invoke_options](../../../doc/DEBUG_API.md#fvm_function_invoke_options) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_function_set_body`](../../../include/flashvm/function.h#L49) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_jit_available`](../../../include/flashvm/jit.h#L15) | [DEBUG_API.md#fvm_jit_available](../../../doc/DEBUG_API.md#fvm_jit_available) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_jit_backend_name`](../../../include/flashvm/jit.h#L19) | [DEBUG_API.md#fvm_jit_backend_name](../../../doc/DEBUG_API.md#fvm_jit_backend_name) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_object_create`](../../../include/flashvm/value.h#L84) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set`](../../../include/flashvm/value.h#L106) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L22) | [STANDARD_OBJECTS.md](../../../doc/STANDARD_OBJECTS.md) — Стандартные классы и объектная модель. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L50) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L46) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-execution_modes).
<!-- example-catalog-api:end -->

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
| `fvm_exception_clear` | function | [include/flashvm/exception.h:74](../../../include/flashvm/exception.h#L74) |
| `fvm_exception_get` | function | [include/flashvm/exception.h:66](../../../include/flashvm/exception.h#L66) |
| `fvm_exception_raise` | function | [include/flashvm/exception.h:78](../../../include/flashvm/exception.h#L78) |
| `fvm_execution_get_stats` | function | [include/flashvm/execution_mode.h:92](../../../include/flashvm/execution_mode.h#L92) |
| `fvm_execution_options_init` | function | [include/flashvm/execution_mode.h:74](../../../include/flashvm/execution_mode.h#L74) |
| `fvm_execution_set_mode` | function | [include/flashvm/execution_mode.h:79](../../../include/flashvm/execution_mode.h#L79) |
| `fvm_function_create_avm1` | function | [include/flashvm/function.h:62](../../../include/flashvm/function.h#L62) |
| `fvm_function_invoke_options` | function | [include/flashvm/execution_mode.h:101](../../../include/flashvm/execution_mode.h#L101) |
| `fvm_function_set_body` | function | [include/flashvm/function.h:49](../../../include/flashvm/function.h#L49) |
| `fvm_jit_available` | function | [include/flashvm/jit.h:15](../../../include/flashvm/jit.h#L15) |
| `fvm_jit_backend_name` | function | [include/flashvm/jit.h:19](../../../include/flashvm/jit.h#L19) |
| `fvm_object_create` | function | [include/flashvm/value.h:84](../../../include/flashvm/value.h#L84) |
| `fvm_object_set` | function | [include/flashvm/value.h:106](../../../include/flashvm/value.h#L106) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../../../include/flashvm/standard.h#L22) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
<!-- source-links:end -->
