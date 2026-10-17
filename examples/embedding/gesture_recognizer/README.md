# Touch recognizer / Распознавание касаний

**EN:** `main.c` demonstrates two-finger tap -> right mouse button using the
host-driven recognizer. Input timestamps are monotonic milliseconds; coordinates
are stage pixels. `BEGIN` starts a sequence, `MOVE` updates its maximum finger
count, `END` recognizes and dispatches, `CANCEL` discards. This is a host-side
callback, **not** an ActionScript `onPress` event yet.

**RU:** `main.c` показывает преобразование тапа двумя пальцами в правую кнопку
мыши. Время передаётся в монотонных миллисекундах, координаты — в пикселях
сцены. `BEGIN` начинает жест, `MOVE` обновляет максимальное количество пальцев,
`END` распознаёт жест, `CANCEL` отменяет. Событие пока поступает в C callback,
**не** в ActionScript `onPress`.

Build / Сборка: enable `FVM_BUILD_EXAMPLES=ON` in CMake.

<!-- example-catalog-api:start -->
## Назначение и особенности

Подаёт BEGIN/MOVE/END/CANCEL в host-распознаватель и преобразует двухпальцевый tap в кнопку мыши. Объясняет монотонное время, координаты сцены и отличия распознавания от непосредственного AS-события.

**Текущая CMake-цель:** `flashvm_examples_embedding_gesture_recognizer_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_gesture_recognizer_feed`](../../../include/flashvm/gesture_recognizer.h) | [INPUT_EVENTS.md](../../../doc/INPUT_EVENTS.md) — Распознавание и передача жестов. |
| [`fvm_gesture_recognizer_init`](../../../include/flashvm/gesture_recognizer.h) | [INPUT_EVENTS.md](../../../doc/INPUT_EVENTS.md) — Распознавание и передача жестов. |
| [`fvm_input_set_callback`](../../../include/flashvm/input.h) | [INPUT_API.md](../../../doc/INPUT_API.md) — Правила преобразования ввода. |
| [`fvm_input_set_rule`](../../../include/flashvm/input.h) | [INPUT_API.md](../../../doc/INPUT_API.md) — Правила преобразования ввода. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |

[Описание в общем каталоге](../../README.md#example-embedding-gesture_recognizer).
<!-- example-catalog-api:end -->
