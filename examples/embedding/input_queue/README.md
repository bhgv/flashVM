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
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_input_enqueue`](../../../include/flashvm/input_queue.h) | [INPUT_QUEUE.md](../../../doc/INPUT_QUEUE.md) — Очередь событий и pump на VM-потоке. |
| [`fvm_input_pump`](../../../include/flashvm/input_queue.h) | [INPUT_QUEUE.md](../../../doc/INPUT_QUEUE.md) — Очередь событий и pump на VM-потоке. |
| [`fvm_input_set_callback`](../../../include/flashvm/input.h) | [INPUT_API.md](../../../doc/INPUT_API.md) — Правила преобразования ввода. |
| [`fvm_input_set_rule`](../../../include/flashvm/input.h) | [INPUT_API.md](../../../doc/INPUT_API.md) — Правила преобразования ввода. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |

[Описание в общем каталоге](../../README.md#example-embedding-input_queue).
<!-- example-catalog-api:end -->
