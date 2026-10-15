# StartDrag / EndDrag execution — v0.329

AVM1 StartDrag (0x27) resolves the top target before converting the center and
constraint flags to signed Int32. Only integer one enables either flag. When
constrained, limits convert in pop order yMax, xMax, yMin, xMin. The adapter
retains the resolved native target through later collecting callbacks and calls
the shared C drag engine after preparation. It consumes its operands without
pushing a result. EndDrag (0x28) flushes deferred motion and ends context-wide
dragging through the same C service.

Loaded missing operands are padded below existing stack values; host-only
execution stays strict. Constraints consume four values only when enabled.
An unresolved source still converts flags/limits but leaves any existing drag
unchanged. Empty/undefined source paths retain existing versioned resolution.
All flag/number conversion, path resolution and drag motion use C services;
only encoded operand handling and start-target selection stay in the interpreter.

The native limits, parent transforms, active GC root, atomic motion, removal,
callback cancellation/replacement and deferred stop rules are documented in
NATIVE_DRAG_SERVICE.md. The script startDrag/stopDrag methods are not registered
yet. Drop-target picking, full display types, exact twips, singular parent
fallback and complete event ordering remain open. No JIT backend is introduced.

Primary comparison: Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd,
activation.rs action_start_drag/action_end_drag, globals/movie_clip.rs
start_drag_impl/stop_drag and player.rs update_drag. Source comparison and
synthetic tests do not certify independent Flash Player equivalence.

avm1_drag exercises host/SWF4–8 raw bytes, constrained and delta motion, exact
flag-one policy, stack consumption, missing targets/operands and stop flushing.
avm1_drag_callbacks checks all seven coercion steps, exceptions at each step,
GC during conversion and real detachment during the center conversion. The
resolved clip survives solely through the adapter root until native drag begins;
then the drag context root keeps it alive until removal update cancels it.
avm1_drag_allocation enumerates StartDrag/EndDrag allocation failures, restoring
activation/root/native/update scopes and accepting documented completed-operation
side effects if a later interpreter safe point fails.

## Русский

Подключены Action StartDrag и EndDrag. Сначала разрешается цель, затем флаги
преобразуются в Int32; включает флаг только значение 1. Ограничения читаются в
порядке yMax, xMax, yMin, xMin. Цель удерживается временным корнем при последующих
преобразованиях, включая GC после её реального удаления со сцены. Затем начинает
работать общий C-механизм, сохраняющий цель своим корнем.

Операнды потребляются без результата на стеке. Для загруженного SWF пропуски
дополняются undefined, host остаётся строгим. EndDrag выполняет отложенное
перемещение и останавливает drag. Проверены raw-байткод, порядок преобразований,
исключения на каждом шаге, GC, удалённая цель и отказы памяти. Скриптовые методы
startDrag/stopDrag пока не зарегистрированы. Полные события, drop-target,
точность twips и другие открытые ограничения не объявляются завершёнными.

### v0.330 follow-up / Продолжение

Native startDrag/stopDrag adapters now share script-style C option preparation;
see MOVIECLIP_DRAG.md. Their Boolean/forward-limit semantics remain distinct
from the Action's integer flags/reverse stack-pop order. Full gaps stay open.
В v0.330 подключены скриптовые адаптеры; полные ограничения совместимости остаются.
