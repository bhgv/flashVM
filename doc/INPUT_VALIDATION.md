# Consistent host input validation / Общая проверка ввода — v0.323

Direct mapping/dispatch previously accepted gesture values and finger counts
that enqueue already rejected. All three event entry points now use one private
validator before mapping, callback delivery or copying an event into the queue.
Supported gestures are TAP through WHEEL, finger counts are 1 through
FVM_INPUT_MAX_FINGERS, and both coordinates must be finite. Direct dispatch and
enqueue also share phase validation. Invalid mapping leaves its output unchanged;
invalid dispatch invokes no callback; invalid enqueue preserves the existing FIFO.

The public C89 input_limits.h documents FVM_INPUT_MAX_FINGERS (16) and
FVM_INPUT_MAX_PENDING_EVENTS (1024), replacing repeated numeric literals in input
rules, event validation and queue capacity. The API preserves custom direction
values, exact-before-wildcard rule precedence, unbound-event notification,
FIFO ordering and owning-thread-only queue use. No new cross-thread guarantees
or MovieClip drag/mouse event integration are claimed.

input_validation is C89. It covers zero/excess fingers, invalid gestures/phases,
positive/negative infinity and NaN on each coordinate; checks unchanged mapping
output, queue contents and callback count on rejection; exercises every supported
gesture/finger/phase tuple; and checks capacity rejection, freeing one slot,
refilling and draining without losing accepted events. Existing input, dispatch,
queue and recognizer tests provide regression coverage.

## Русский

Прямой ввод и очередь теперь используют одну проверку: известный жест,
от одного до FVM_INPUT_MAX_FINGERS пальцев и конечные координаты. Фазы также
проверяются одинаково. Ошибка не вызывает callback, не меняет очередь и
сохраняет выход map. Раньше прямой ввод пропускал случаи, запрещённые очередью.

Пределы 16 пальцев и 1024 ожидающих событий вынесены в публичный C89-заголовок
input_limits.h с комментариями. Сохранены порядок правил, направление,
доставка несопоставленных жестов и FIFO. Тест проверяет неверные значения,
NaN/бесконечности, все допустимые комбинации и границы ёмкости очереди.
Полное подключение MovieClip drag/mouse и других стадий остаётся открытым.
