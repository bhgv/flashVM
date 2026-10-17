# Gesture recognizer API

The C89 header `flashvm/gesture_recognizer.h` declares a host-driven touch
recognizer independent of operating-system input APIs. The host supplies a
monotonic millisecond timestamp, centroid coordinates in stage pixels, and the
current number of contacts. A gesture begins with [`FVM_TOUCH_BEGIN`](../include/flashvm/gesture_recognizer.h#L19) and ends
with [`FVM_TOUCH_END`](../include/flashvm/gesture_recognizer.h#L21) or [`FVM_TOUCH_CANCEL`](../include/flashvm/gesture_recognizer.h#L22). The maximum observed contact count
is used for multi-tap mapping. The recognizer classifies a tap when distance is
at most `tap_max_distance` (default 12 px) and duration is at most
`tap_max_duration_ms` (default 350 ms). A swipe requires distance at least
`swipe_min_distance` (default 48 px). Swipe directions: -1 left, +1 right,
-2 up, +2 down. Gesture events use [`fvm_input_dispatch`](../include/flashvm/input.h#L82) and are delivered to
registered C callbacks at [`FVM_INPUT_END`](../include/flashvm/input.h#L64). Mouse mapping uses existing
[`fvm_input_set_rule`](../include/flashvm/input.h#L52). `dispatched` reports recognition, even if no mapping exists.

The recognizer is per gesture stream, not thread-safe, and does not identify
individual contact IDs or track simultaneous independent gesture sequences.
No operating-system polling, mouse injection, AVM1 handler delivery, or
multi-click timing semantics are implemented by this module. Avoid feeding
untrusted or non-monotonic timestamps; invalid samples return an error.

# API распознавания жестов

Заголовок C89 `flashvm/gesture_recognizer.h` описывает независимый от ОС
распознаватель. Приложение передаёт монотонное время в миллисекундах,
координаты центра касаний в пикселях сцены и число активных пальцев.
[`FVM_TOUCH_BEGIN`](../include/flashvm/gesture_recognizer.h#L19) начинает жест, [`FVM_TOUCH_END`](../include/flashvm/gesture_recognizer.h#L21) завершает, [`FVM_TOUCH_CANCEL`](../include/flashvm/gesture_recognizer.h#L22)
отменяет. Для мультитапа используется максимальное число пальцев за жест.

Тап: расстояние не более `tap_max_distance` (по умолчанию 12 пикселей),
длительность не более `tap_max_duration_ms` (350 мс). Свайп: расстояние не
менее `swipe_min_distance` (48 пикселей). Направления: -1 влево, +1 вправо,
-2 вверх, +2 вниз. События поступают в зарегистрированный C-обработчик через
[`fvm_input_dispatch`](../include/flashvm/input.h#L82) с фазой [`FVM_INPUT_END`](../include/flashvm/input.h#L64). Правила эмуляции кнопок мыши
задаются [`fvm_input_set_rule`](../include/flashvm/input.h#L52). `dispatched=1` означает, что жест распознан,
даже если правило кнопки не задано.

Распознаватель обслуживает одну последовательность жестов, не является
потокобезопасным, не отслеживает ID каждого пальца и не распознаёт независимые
одновременные жесты. Передача в обработчики AVM1, системная инъекция мыши и
опрос ОС пока отсутствуют. Пример: `examples/embedding/gesture_recognizer/`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_dispatch` | function | [include/flashvm/input.h:82](../include/flashvm/input.h#L82) |
| `FVM_INPUT_END` | enum constant | [include/flashvm/input.h:64](../include/flashvm/input.h#L64) |
| `fvm_input_set_rule` | function | [include/flashvm/input.h:52](../include/flashvm/input.h#L52) |
| `FVM_TOUCH_BEGIN` | enum constant | [include/flashvm/gesture_recognizer.h:19](../include/flashvm/gesture_recognizer.h#L19) |
| `FVM_TOUCH_CANCEL` | enum constant | [include/flashvm/gesture_recognizer.h:22](../include/flashvm/gesture_recognizer.h#L22) |
| `FVM_TOUCH_END` | enum constant | [include/flashvm/gesture_recognizer.h:21](../include/flashvm/gesture_recognizer.h#L21) |
<!-- source-links:end -->
