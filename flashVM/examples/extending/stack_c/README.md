# extending/stack_c

Регистрирует callback со стеком [FVM_Stack](../../../src/capi/stack_c.cpp#L16), читает аргументы, очищает стек и возвращает сумму. Показывает индексацию, размер стека и правило единственного результата.

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
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_stack_call`](../../../include/flashvm/stack.h#L50) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_clear`](../../../include/flashvm/stack.h#L33) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_get`](../../../include/flashvm/stack.h#L25) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_push`](../../../include/flashvm/stack.h#L29) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_register`](../../../include/flashvm/stack.h#L35) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_stack_size`](../../../include/flashvm/stack.h#L22) | [STACK_API.md#c-interface](../../../doc/STACK_API.md#c-interface) — Индексы стека и контракт native callback. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-extending-stack_c).
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
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `FVM_Stack` | struct | [src/capi/stack_c.cpp:16](../../../src/capi/stack_c.cpp#L16) |
| `fvm_stack_call` | function | [include/flashvm/stack.h:50](../../../include/flashvm/stack.h#L50) |
| `fvm_stack_clear` | function | [include/flashvm/stack.h:33](../../../include/flashvm/stack.h#L33) |
| `fvm_stack_get` | function | [include/flashvm/stack.h:25](../../../include/flashvm/stack.h#L25) |
| `fvm_stack_push` | function | [include/flashvm/stack.h:29](../../../include/flashvm/stack.h#L29) |
| `fvm_stack_register` | function | [include/flashvm/stack.h:35](../../../include/flashvm/stack.h#L35) |
| `fvm_stack_size` | function | [include/flashvm/stack.h:22](../../../include/flashvm/stack.h#L22) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
<!-- source-links:end -->
