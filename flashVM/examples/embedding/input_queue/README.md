# embedding/input_queue

Помещает распознанные события в отложенную очередь и передаёт их callback при pump на VM-потоке. Позволяет отделить получение ввода от его обработки без графического backend.

## Реализация и запуск

Пример C/C++ собирается с `FVM_BUILD_EXAMPLES=ON`. Из корня репозитория:

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-examples --target flashvm_examples_embedding_input_queue_main --parallel
./build-examples/flashvm_examples_embedding_input_queue_main
```

Операции зависят от выбранного DEFS-профиля; OS_PORTS, JIT, графика и codecs
не включаются наличием этого README. Исходники ниже показывают порядок API-вызовов
и освобождения ресурсов.

<!-- example-catalog-api:start -->
**Текущая CMake-цель:** `flashvm_examples_embedding_input_queue_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_input_enqueue`](../../../include/flashvm/input_queue.h#L22) | [INPUT_QUEUE.md](../../../doc/INPUT_QUEUE.md) — Очередь событий и pump на VM-потоке. |
| [`fvm_input_pump`](../../../include/flashvm/input_queue.h#L45) | [INPUT_QUEUE.md](../../../doc/INPUT_QUEUE.md) — Очередь событий и pump на VM-потоке. |
| [`fvm_input_set_callback`](../../../include/flashvm/input.h#L69) | [INPUT_API.md](../../../doc/INPUT_API.md) — Правила преобразования ввода. |
| [`fvm_input_set_rule`](../../../include/flashvm/input.h#L52) | [INPUT_API.md](../../../doc/INPUT_API.md) — Правила преобразования ввода. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |

[Описание в общем каталоге](../../README.md#example-embedding-input_queue).
<!-- example-catalog-api:end -->
## v0.603 entry batch / Начальный пакет

The queue now also supports copied Mouse/Key variants, documented by the
[mixed tutorial](../mixed_input/README.md). Pump with max_events=0 delivers only
the entry batch; callback additions wait for the next pump. The gesture-only
example above keeps its behavior because it does not append from callbacks.
По-русски: ноль теперь означает пакет на входе, новые события из callback
ждут следующего pump. Очистка очереди и сброс polling — разные операции.

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
| `fvm_input_enqueue` | function | [include/flashvm/input_queue.h:22](../../../include/flashvm/input_queue.h#L22) |
| `fvm_input_pump` | function | [include/flashvm/input_queue.h:45](../../../include/flashvm/input_queue.h#L45) |
| `fvm_input_set_callback` | function | [include/flashvm/input.h:69](../../../include/flashvm/input.h#L69) |
| `fvm_input_set_rule` | function | [include/flashvm/input.h:52](../../../include/flashvm/input.h#L52) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
<!-- source-links:end -->
