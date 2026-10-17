# Gesture dispatch / Передача жестов

**EN:** `main.c` is a working C89 example of the host-facing gesture callback. A two-finger tap is mapped to the right mouse button and dispatched synchronously. The accompanying `.as` file is a **future API sketch**, not executable on the current AVM1 runtime. See `doc/INPUT_EVENTS.md`.

**RU:** `main.c` — работающий пример C89: тап двумя пальцами сопоставляется с правой кнопкой мыши и синхронно передаётся C-обработчику. Файл `.as` — **эскиз будущего API**, текущий интерпретатор его не выполняет. См. `doc/INPUT_EVENTS.md`.

<!-- example-catalog-api:start -->
## Назначение и особенности

Сопоставляет распознанный двухпальцевый tap с правой кнопкой мыши и синхронно передаёт событие C callback. Показывает именно host dispatch; сопровождающий AS-файл является отдельным эскизом.

**Текущая CMake-цель:** `flashvm_examples_embedding_gesture_dispatch_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_input_dispatch`](../../../include/flashvm/input.h) | [INPUT_API.md](../../../doc/INPUT_API.md) — Правила преобразования ввода. |
| [`fvm_input_set_callback`](../../../include/flashvm/input.h) | [INPUT_API.md](../../../doc/INPUT_API.md) — Правила преобразования ввода. |
| [`fvm_input_set_rule`](../../../include/flashvm/input.h) | [INPUT_API.md](../../../doc/INPUT_API.md) — Правила преобразования ввода. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |

[Описание в общем каталоге](../../README.md#example-embedding-gesture_dispatch).
<!-- example-catalog-api:end -->
