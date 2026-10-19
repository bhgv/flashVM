# Deferred mixed input queue / Отложенная смешанная очередь ввода

## English contract (v0.603, wheel extension v0.606)

`flashvm/input_queue.h` exposes one per-context FIFO for recognized gestures,
primary Mouse transitions, wheel deltas and complete Key snapshots. All variants preserve
capture order together. It contains native copies only, no borrowed event,
managed object, SDK handle or callback userdata. No window SDK is required.
Capacity is [`FVM_INPUT_MAX_PENDING_EVENTS`](../include/flashvm/input_limits.h#L7) (1024 total entries, not per variant).
Storage grows through the VM's native vector; enqueue may allocate, and copied
keyboard entries increase per-entry storage compared with the old gesture-only
queue. This is not a lock-free or hard-real-time API.

All operations, including enqueue, run on the VM-owning thread. This queue is
**not thread-safe**. An OS/Android input worker must first marshal copied data
through its host-owned thread-safe channel; do not call VM enqueue there.
Collector/teardown reentry is invalid. A callback may append, inspect or clear
the queue but cannot pump recursively or destroy the runtime. Context destruction
is blocked for the whole pump; no managed objects need rooting for the stored
payloads. Script recipients/GC roots remain the normal dispatchers' responsibility.

### Capture functions

| Function | Arguments and behavior |
| --- | --- |
| [`fvm_input_enqueue`](../include/flashvm/input_queue.h#L22) | Context, borrowed finite [`FVM_InputEvent`](../include/flashvm/input.h#L42), supported phase: copy a recognized gesture. Existing native gesture callback/mouse-rule mapping is used on dispatch; no automatic Mouse broadcaster notification is implied. |
| [`fvm_input_enqueue_mouse`](../include/flashvm/input_queue.h#L25) | Context, [`FVM_MouseEventType`](../include/flashvm/mouse.h#L11) DOWN/MOVE/UP, finite world x/y: copy a primary contact transition. The ordinary Mouse dispatcher updates pointer/drag before registered listeners on delivery. |
| [`fvm_input_enqueue_keyboard`](../include/flashvm/input_queue.h#L34) | Context, borrowed valid [`FVM_KeyboardState`](../include/flashvm/keyboard.h#L50), pressed exactly 0/1, finite event-time world x/y: copy all native key fields and pointer coordinates. On delivery publish the saved pointer, then call the normal rooted Key dispatcher (selected clip before listeners). |

No capture call runs a handler or changes polling/pointer state. Producer
storage may be reused immediately after return. Coordinates must already be
world pixels, converted **once** using the backend's actual event transform;
use [`fvm_host_pointer_to_world`](../include/flashvm/host.h#L26) where appropriate. For a keyboard transition
capture the host's pointer position at that event, not a later final-frame state.
Snapshots retain no managed roots and cannot turn into a dangling SDK pointer.

Valid capture returns [`FVM_OK`](../include/flashvm/flashvm.h#L42). Invalid context/type/state/phase/nonfinite
coordinates return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), full capacity returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L43),
native allocation failure returns [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44); old entries and input
state are unchanged on these failures. Overflow is not silently dropped or
latched forever: host must handle the returned failure, then drain/cancel by
its policy. Unexpected native append exceptions are contained as [`FVM_ERROR`](../include/flashvm/flashvm.h#L43).

### Pump, query and cancellation

`fvm_input_pump(context, max_events, processed)` is synchronous. It dispatches
only entries present **on entry**, in FIFO order, up to max_events. Zero means
the entire **entry batch**, not an unlimited stream including callback additions.
This v0.603 bound intentionally replaces the previous gesture-only pump's
potentially unbounded drain when handlers enqueue more events. Additions wait
for the next pump, even when spare capacity remains in the current batch.

The current item is copied and removed before dispatch. On real callback/script
error it stays consumed, published state stays committed, later entries remain,
and that error returns unchanged. `processed` counts only successful deliveries;
it may be NULL and is set to zero before invalid-context/recursive-call checks.
Dispatch exceptions follow each normal dispatcher's C-boundary contract.
The pump guard resets on return/unwinding. Pump does not clear script exceptions:
the owner handles them using the normal exception API before retrying retained
entries. An empty queue returns success with zero processed.

`fvm_input_pending(context, count)` copies the total count across variants;
count must be non-NULL. Invalid arguments preserve its output.

`fvm_input_clear_queue(context)` drops **all pending** variants without
callbacks/allocation and invalidates the old batch even if invoked from its
current callback. New entries appended after clear wait for another pump;
they are not mistaken for cancelled old entries. The current callback still
completes normally and counts if it succeeds. This operation returns
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) on unusable context, otherwise [`FVM_OK`](../include/flashvm/flashvm.h#L42).

Clear does not synthesize Mouse-up, cancel, click, Key-up, change focus, reset
held keys/toggles or rewind committed pointer/drag state. The host separately
chooses cancellation/reset/synthetic-event policy. No automatic focus, wheel,
raw multitouch recognition, IME, ClipActions or OS polling is added. Existing
SDL player FIFO and direct SDK tutorials remain separate adapter code; this
change does not claim their automatic migration or native-device acceptance.

Tutorials: [mixed Mouse/Key](../examples/embedding/mixed_input/README.md),
[recognized gestures](../examples/embedding/input_queue/README.md).
The latter's example.as is a future script-event sketch, not executable API.

## Русский контракт

С v0.603 один FIFO контекста сохраняет порядок распознанных жестов, Mouse и
Key. В очереди только копии нативных данных, без указателей на managed-объекты,
платформенные события и userdata. Ёмкость 1024 относится ко всем видам вместе.
Вектор может выделять память; полный клавиатурный снимок увеличивает размер
записи. Это не lock-free очередь и не жёсткий real-time контракт.

Все операции, даже enqueue, выполняет поток-владелец VM. Очередь не
потокобезопасна: системный поток передаёт копии через очередь самого хоста.
Вход из GC/уничтожения контекста запрещён. Обработчик может добавлять, читать
или очищать очередь, но не вызывать pump рекурсивно и не уничтожать runtime.
Уничтожение context блокируется на всё время pump. Хранимым данным GC-корни
не нужны; корни скриптовых получателей обеспечивает обычный диспетчер.

[`fvm_input_enqueue`](../include/flashvm/input_queue.h#L22) копирует конечное событие распознанного жеста и phase.
При доставке используется прежний callback жестов и его mouse-rule mapping,
а не автоматическая рассылка Mouse. [`fvm_input_enqueue_mouse`](../include/flashvm/input_queue.h#L25) копирует DOWN,
MOVE или UP с конечными мировыми координатами. [`fvm_input_enqueue_keyboard`](../include/flashvm/input_queue.h#L34)
копирует проверенный полный снимок клавиш, pressed=0/1 и мировую позицию
указателя в момент этого перехода. При доставке сначала публикуется сохранённая
позиция, затем вызывается обычная Key-доставка с фокусным клипом и слушателями.
До pump ни polling, ни указатель, ни callbacks не меняются; входную память
можно сразу переиспользовать. Преобразование координат backend выполняет
один раз до постановки. Нельзя подменять событие более поздним финальным
состоянием кадра.

Успех — [`FVM_OK`](../include/flashvm/flashvm.h#L42), неверные аргументы — [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), полная очередь —
[`FVM_ERROR`](../include/flashvm/flashvm.h#L43), нехватка нативной памяти — [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44). При ошибке
постановки прежние записи и состояния сохраняются. Неожиданное исключение
append преобразуется в [`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Переполнение не теряется молча и не
делает очередь навсегда неисправной: хост обрабатывает ошибку и выбирает политику.

[`fvm_input_pump`](../include/flashvm/input_queue.h#L45) обрабатывает только начальный пакет: max_events ограничивает
число доставок, ноль означает весь пакет на входе. Это намеренное изменение
v0.603: обработчик больше не может бесконечно продлевать один pump новыми
событиями. Добавленное во время доставки ждёт следующего pump.

Текущая запись извлекается до callback. При настоящей ошибке она считается
потреблённой, опубликованное состояние не откатывается, остаток сохраняется,
ошибка возвращается как есть. processed может быть NULL; он считает только
успешные доставки и обнуляется перед проверкой ошибочного/рекурсивного вызова.
Защита pump снимается и при ошибке. Скриптовое исключение очищает владелец
через обычный exception API, а не очередь. Пустая очередь успешно даёт ноль.

[`fvm_input_pending`](../include/flashvm/input_queue.h#L48) возвращает общее число записей через ненулевой count;
ошибка сохраняет выход. [`fvm_input_clear_queue`](../include/flashvm/input_queue.h#L39) без callbacks и выделений
удаляет все ожидающие виды и отменяет остаток активного пакета. Новые записи,
добавленные после clear внутри callback, ждут нового pump. Текущий callback
завершается и учитывается, если успешен. Clear не создаёт UP/click/cancel,
не меняет фокус, уже опубликованные координаты/drag и не сбрасывает клавиши;
это отдельная политика хоста. Не добавлены IME, wheel, ClipActions,
распознавание multitouch и опрос OS. Нативный SDL FIFO и прямые SDK-примеры
автоматически не мигрированы. Приёмка телефонов и всех стадий не заявляется.

## Queued wheel / Колесо в очереди (v0.606)

[fvm_input_enqueue_mouse_wheel](../include/flashvm/input_queue.h#L30)(context, delta, world_x, world_y) borrows an owning-
thread live context outside GC/release/destruction, and copies finite signed
line `delta` plus finite event-time world coordinates. No callback, pointer
publication or drag update occurs at enqueue. Fractional/zero delta is retained.
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) preserves the FIFO for invalid context/state or NaN/Infinity
in any input. [FVM_ERROR](../include/flashvm/flashvm.h#L43) at the shared 1024-item capacity and [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44)
on append preserve every existing entry. Unexpected native exceptions map to
[FVM_ERROR](../include/flashvm/flashvm.h#L43). No managed pointer, native platform event or scrollTarget is stored.

Wheel shares **one ordered FIFO** with gestures, primary pointer and Key entries.
Pump uses the normal rooted [fvm_input_dispatch_mouse_wheel](../include/flashvm/mouse.h#L32), including its actual
handler status/exception. The failing event is consumed, processed counts only
successful events, and later entries remain queued. Clear the VM exception
explicitly before retry where required. Entry-batch limit, callback additions,
cancellation epoch, recursion/teardown guards and clear_queue behavior are the
same for all variants. Clear drops queued wheel too, but never synthesizes a
counter-wheel, button release, focus change or keyboard reset. No coalescing or
delta summation occurs; even opposite deltas remain separate ordered events.

Русский: новая функция копирует конечные delta (строки со знаком) и координаты
на момент события. Очередь общая с жестами, Mouse и Key; заимствованных указателей
и новых корней GC нет. Enqueue не меняет текущую позицию и не вызывает скрипт.
Неверный ввод, переполнение и OOM не портят уже поставленные события. При pump
доставка проходит через обычный укоренённый Mouse-dispatcher. Ошибочное событие
снимается, последующие остаются; счётчик processed учитывает только успешные.
Добавленные из обработчика события ждут следующего pump; отмена прерывает старую
партию и сохраняет события, добавленные после отмены. Противоположные дельты
не схлопываются. Самостоятельный хост нормализует единицы до enqueue.

[C89 wheel tutorial](../examples/embedding/mouse_wheel/README.md),
[direct wheel contract](MOUSE.md#wheel-events), [RU](MOUSE.ru.md#колесо-прокрутки).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_host_pointer_to_world` | function | [include/flashvm/host.h:26](../include/flashvm/host.h#L26) |
| `fvm_input_clear_queue` | function | [include/flashvm/input_queue.h:39](../include/flashvm/input_queue.h#L39) |
| `fvm_input_dispatch_mouse_wheel` | function | [include/flashvm/mouse.h:32](../include/flashvm/mouse.h#L32) |
| `fvm_input_enqueue` | function | [include/flashvm/input_queue.h:22](../include/flashvm/input_queue.h#L22) |
| `fvm_input_enqueue_keyboard` | function | [include/flashvm/input_queue.h:34](../include/flashvm/input_queue.h#L34) |
| `fvm_input_enqueue_mouse` | function | [include/flashvm/input_queue.h:25](../include/flashvm/input_queue.h#L25) |
| `fvm_input_enqueue_mouse_wheel` | function | [include/flashvm/input_queue.h:30](../include/flashvm/input_queue.h#L30) |
| `FVM_INPUT_MAX_PENDING_EVENTS` | macro / define | [include/flashvm/input_limits.h:7](../include/flashvm/input_limits.h#L7) |
| `fvm_input_pending` | function | [include/flashvm/input_queue.h:48](../include/flashvm/input_queue.h#L48) |
| `fvm_input_pump` | function | [include/flashvm/input_queue.h:45](../include/flashvm/input_queue.h#L45) |
| `FVM_InputEvent` | struct | [include/flashvm/input.h:42](../include/flashvm/input.h#L42) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_KeyboardState` | struct | [include/flashvm/keyboard.h:50](../include/flashvm/keyboard.h#L50) |
| `FVM_MouseEventType` | enum | [include/flashvm/mouse.h:11](../include/flashvm/mouse.h#L11) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
