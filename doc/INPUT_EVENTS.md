# Input events / События ввода

## English

FlashVM accepts gestures already recognized by the host. `fvm_input_set_rule` maps a gesture, finger count and direction to a synthetic mouse button. The first matching exact direction takes precedence over a wildcard direction (`0`). Direction values: `-1` left, `1` right, `-2` up, `2` down. `FVM_MOUSE_WHEEL_UP` and `FVM_MOUSE_WHEEL_DOWN` are wheel actions, not held buttons. Up to 16 fingers can be configured.

`fvm_input_set_callback` registers a synchronous C callback. `fvm_input_dispatch` delivers every gesture to that callback, whether or not it matches a mouse rule. The callback receives the original gesture, `BEGIN` / `UPDATE` / `END` phase, and the mapped mouse action. Set a NULL callback to unregister. Do not destroy the context inside the callback. The host owns gesture recognition, coordinates, and sequencing; FlashVM does not infer swipes from raw touch points.

**Current limitation:** this callback is host-facing only; there is no automatic AVM1 `onPress`, `onRelease`, `onMouseMove`, `onTap`, `onMultiTap`, or `onSwipe` dispatch. A future display-list hit-test and event dispatcher will connect these APIs. Mouse mapping is not equivalent to delivering mouse events to a MovieClip.

## Русский

FlashVM принимает жесты, уже распознанные приложением-хостом. `fvm_input_set_rule` сопоставляет тип жеста, количество пальцев и направление с синтетическим действием мыши. Точное совпадение направления имеет приоритет перед универсальным (`0`). Направления: `-1` — влево, `1` — вправо, `-2` — вверх, `2` — вниз. `FVM_MOUSE_WHEEL_UP` и `FVM_MOUSE_WHEEL_DOWN` обозначают прокрутку, а не удерживаемые кнопки. Допускаются правила для 1–16 пальцев.

`fvm_input_set_callback` регистрирует синхронный C-обработчик. `fvm_input_dispatch` передаёт ему каждый жест, даже если подходящего правила мыши нет. Обработчик получает исходный жест, фазу `BEGIN` / `UPDATE` / `END` и назначенное действие мыши. Для отмены регистрации передайте NULL. Не уничтожайте контекст внутри обработчика. Распознавание жестов, координаты и последовательность фаз обеспечивает приложение-хост; FlashVM пока не вычисляет свайпы по сырым касаниям.

**Текущее ограничение:** события пока передаются только C-обработчику хоста. Автоматического вызова AVM1 `onPress`, `onRelease`, `onMouseMove`, `onTap`, `onMultiTap` или `onSwipe` нет. Для этого необходимы hit-testing и диспетчер событий DisplayList. Сопоставление с кнопкой мыши само по себе не означает доставку события MovieClip.
