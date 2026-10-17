# embedding/jit

Явно компилирует тело x*2+2 и подключает его к обычной AVM1-функции. Сравнивает результат вызова и размер кода, показывает отдельное владение [FVM_JitCode](../../../src/jit/internal.hpp#L59) и interpreter fallback.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal-jit.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_jit_main --parallel
./build-examples/flashvm_examples_embedding_jit_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_jit_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_function_create_avm1`](../../../include/flashvm/function.h#L52) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_function_invoke`](../../../include/flashvm/function.h#L65) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_function_set_body`](../../../include/flashvm/function.h#L43) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_jit_code_size`](../../../include/flashvm/jit.h#L35) | [DEBUG_API.md#fvm_jit_code_size](../../../doc/DEBUG_API.md#fvm_jit_code_size) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_jit_compile`](../../../include/flashvm/jit.h#L21) | [JIT.md#lifetime-gc-and-failures](../../../doc/JIT.md#lifetime-gc-and-failures) — JIT-код, исходное тело и время жизни. |
| [`fvm_jit_destroy`](../../../include/flashvm/jit.h#L28) | [JIT.md#lifetime-gc-and-failures](../../../doc/JIT.md#lifetime-gc-and-failures) — JIT-код, исходное тело и время жизни. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L18) | [STANDARD_OBJECTS.md](../../../doc/STANDARD_OBJECTS.md) — Стандартные классы и объектная модель. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-jit).
<!-- example-catalog-api:end -->


Windows x64 offline call-plan encoder and actual Linux `ms_abi` validation / автономный генератор и проверка ABI: [JIT_WINDOWS_X64.md](../../../doc/JIT_WINDOWS_X64.md). v0.561 adds Windows x64 selection, W^X allocation and registered unwind metadata; native Windows execution is still unverified. / Добавлены выбор backend, W^X и unwind; нативный Windows запуск ещё не проверен.


macOS Intel offline encoder and linked-frame validation / автономный генератор и цепочка кадров: [JIT_MACOS_X64.md](../../../doc/JIT_MACOS_X64.md). v0.563 does not enable macOS runtime publication / публикация кода macOS пока не включена.

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
| `fvm_function_create_avm1` | function | [include/flashvm/function.h:52](../../../include/flashvm/function.h#L52) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:65](../../../include/flashvm/function.h#L65) |
| `fvm_function_set_body` | function | [include/flashvm/function.h:43](../../../include/flashvm/function.h#L43) |
| `fvm_jit_code_size` | function | [include/flashvm/jit.h:35](../../../include/flashvm/jit.h#L35) |
| `fvm_jit_compile` | function | [include/flashvm/jit.h:21](../../../include/flashvm/jit.h#L21) |
| `fvm_jit_destroy` | function | [include/flashvm/jit.h:28](../../../include/flashvm/jit.h#L28) |
| `FVM_JitCode` | struct | [src/jit/internal.hpp:59](../../../src/jit/internal.hpp#L59) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:18](../../../include/flashvm/standard.h#L18) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
<!-- source-links:end -->
