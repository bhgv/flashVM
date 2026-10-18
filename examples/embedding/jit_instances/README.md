# embedding/jit_instances

Создаёт две VM с одинаковым исходным телом, но независимыми режимами и кэшами. По умолчанию первая интерпретирует, вторая запрашивает JIT. Опция `--both-jit` требует реального нативного исполнения обеих VM. После уничтожения первой VM пример повторно вызывает вторую и показывает счётчики. Все вызовы чередуются на одном owning thread; managed-объекты между VM не передаются.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal-jit.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_jit_instances_main --parallel
./build-examples/flashvm_examples_embedding_jit_instances_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

## Режимы, результат и время жизни / Modes and lifetime

```sh
./build-examples/flashvm_examples_embedding_jit_instances_main --both-jit
```

Default mode permits interpreter fallback for the second VM. The first VM must
have zero native entries. Strict `--both-jit` requires an available native backend
and positive entry counters for both VMs; an automatic fallback is an error even
when the numeric result is correct. Unknown arguments fail before acquiring VMs.
Both modes destroy VM 0 first, call the survivor again and check result 42. Strict
mode additionally requires its native-entry counter to increase. The example
prints a final `survivor VM 1` line before releasing the remaining owner.

`--both-jit` полезен для проверки общей памяти JIT при независимом владении:
два контекста держат разные автоматические кеши, освобождение одного не должно
повредить второму. Это не пример совместного heap, многопоточного использования
одного контекста или передачи объектов между runtime. Примитивный аргумент можно
использовать повторно; функции всегда вызываются через их собственный контекст.
При частичной ошибке освобождаются только созданные и ещё не уничтоженные owners.

On Linux the emitted example runs with the ordinary native backend. For opt-in
macOS ARM64 use the SDK/deployment/build/signing preparation in
[JIT_MACOS_ARM64.md](../../../doc/JIT_MACOS_ARM64.md). Sign this executable itself;
signing the single-VM tutorial does not sign it. Default ARM64 opt-in is OFF;
build availability does not guarantee MAP_JIT/signing permission. The registered
[host harness](../../../scripts/check_jit_instances_example.py) runs default,
strict and invalid-option processes. Native policy denial is an explicit skip,
not successful native acceptance; a failure after VM 0 has entered native code is
not converted to an unavailable-backend skip.

На этой контрольной точке выполнены реальные Linux Release и ASAN/UBSAN прогоны
обоих режимов и неверного аргумента (LeakSanitizer отключён). Нового нативного
запуска macOS/Windows/Android этим примером не заявляется.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_jit_instances_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Вторая колонка объясняет конкретное действие
функции в примере и указывает руководство с её контрактом.

| Функция | Назначение и документация |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Инициализирует значения конфигурации и размер ABI перед созданием runtime. Подробнее: [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Создаёт контекст с собственными managed-объектами, режимом исполнения и автоматическим JIT-кешем. Подробнее: [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Уничтожает один контекст и его кеш; пример проверяет, что код другой VM продолжает исполняться. Подробнее: [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context). |
| [`fvm_execution_get_stats`](../../../include/flashvm/execution_mode.h#L71) | Читает счётчики реальных native/interpreter entries; перед вызовом задаётся struct_size. Подробнее: [DEBUG_API.md#fvm_execution_get_stats](../../../doc/DEBUG_API.md#fvm_execution_get_stats). |
| [`fvm_execution_set_mode`](../../../include/flashvm/execution_mode.h#L64) | Задаёт политику данного контекста: interpreter или automatic JIT с возможным fallback. Подробнее: [DEBUG_API.md#fvm_execution_set_mode](../../../doc/DEBUG_API.md#fvm_execution_set_mode). |
| [`fvm_function_create_avm1`](../../../include/flashvm/function.h#L52) | Создаёт принадлежащую этому контексту AVM1-функцию с параметром x и копией тела. Подробнее: [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md). |
| [`fvm_function_invoke`](../../../include/flashvm/function.h#L65) | Вызывает функцию в её контексте; результат сравнивается с 42 до и после уничтожения другой VM. Подробнее: [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md). |
| [`fvm_jit_available`](../../../include/flashvm/jit.h#L13) | Проверяет наличие backend и платформенной возможности JIT; разрешение подписи проверяется позднее при компиляции. Подробнее: [JIT.md](../../../doc/JIT.md#contract-and-supported-subset). |
| [`fvm_jit_backend_name`](../../../include/flashvm/jit.h#L15) | Сообщает имя ABI сборки; само имя не доказывает успешную компиляцию или нативное исполнение. Подробнее: [DEBUG_API.md#fvm_jit_backend_name](../../../doc/DEBUG_API.md#fvm_jit_backend_name). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Создаёт независимый runtime-owner; каждый из двух контекстов использует свой runtime. Подробнее: [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Освобождает runtime после его контекста; уже уничтоженные owners обнуляются для безопасного cleanup. Подробнее: [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context). |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | Записывает число и его тег в примитивный аргумент, который можно повторно использовать между вызовами. Подробнее: [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views). |

[Описание в общем каталоге](../../README.md#example-embedding-jit_instances).
<!-- example-catalog-api:end -->

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
| `fvm_execution_get_stats` | function | [include/flashvm/execution_mode.h:71](../../../include/flashvm/execution_mode.h#L71) |
| `fvm_execution_set_mode` | function | [include/flashvm/execution_mode.h:64](../../../include/flashvm/execution_mode.h#L64) |
| `fvm_function_create_avm1` | function | [include/flashvm/function.h:52](../../../include/flashvm/function.h#L52) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:65](../../../include/flashvm/function.h#L65) |
| `fvm_jit_available` | function | [include/flashvm/jit.h:13](../../../include/flashvm/jit.h#L13) |
| `fvm_jit_backend_name` | function | [include/flashvm/jit.h:15](../../../include/flashvm/jit.h#L15) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
<!-- source-links:end -->
