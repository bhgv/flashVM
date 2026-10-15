# MovieClip drag methods / Методы перетаскивания — v0.330

MovieClip.startDrag and stopDrag are now registered as stable native callbacks,
appended to the method table so all earlier callback IDs retain their meaning.
Each adapter occupies its own file. The class prototype returns undefined with
no effect. Any native instance can stop the context-wide drag, regardless of
which instance started it. These callbacks survive channel graph transport;
the destination maintains independent pointer and active drag state.

The C89 fvm_exec_drag_values(context, clip, center, bound_count, bounds) service
prepares script-style options without an operand stack. Center may be NULL and
otherwise projects versioned Boolean truth without valueOf/toString callbacks.
Zero bound values means unconstrained. One through four values enable limits,
converted in forward xMin/yMin/xMax/yMax order; missing trailing values convert
from undefined and normalize through the native engine. Counts above the public
FVM_EXEC_DRAG_BOUND_LIMIT and invalid/foreign inputs are rejected. Native script
adapters ignore arguments after the four limits, as the reference does.

All operands and the receiver remain rooted through number conversion. Direct
C calls permit collecting script conversions. Native method entry keeps its
existing enclosing collector barrier; the shared service never suspends that
barrier. Errors before selection leave the previous drag unless a user callback
changed it. Script side effects and a completed operation before a later outer
error are not rolled back. Motion, constraints, removal and end flushing reuse
the same engine used by StartDrag/EndDrag.

Primary comparison: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/globals/movie_clip.rs start_drag/start_drag_impl/stop_drag.
The method Boolean/forward-bound policy differs intentionally from Action
Int32-equals-one/reverse-pop preparation. This is source comparison and bounded
native evidence, not complete Flash Player compatibility. Drop-target picking,
exact twip/singular-transform rules, full display kinds and event ordering
remain open; no JIT machine code is introduced.

movieclip_drag is strict C89: host/SWF4–8 truth, direct service/native parity,
partial/default options, deferred motion, stop through another instance,
prototype no-ops, invalid counts and channel callback remapping/context isolation.
movieclip_drag_callbacks covers four numeric coercions in order, no center
coercion callbacks, every throwing phase, real detachment and collecting direct
calls vs blocked native reentry. movieclip_drag_allocation enumerates direct and
native preparation failures, restored scopes and complete old/new selection.

## Русский

Методы startDrag/stopDrag подключены к общему C-слою. Идентификаторы добавлены
в конец таблицы без сдвига прежних, адаптеры вынесены по одному в файл. StopDrag
останавливает drag контекста независимо от получателя; прототип класса ничего
не делает и возвращает undefined. Перенос через канал сохраняет callbacks,
но не переносит активный drag и указатель исходного контекста.

Новый C-сервис готовит Boolean-центрирование и до четырёх границ в прямом порядке
xMin/yMin/xMax/yMax. Пропущенные хвостовые границы становятся undefined; любое
наличие границ включает ограничения. Отличие от Action-флагов Int32==1 и обратного
порядка pop сохранено. Входы и клип удерживаются корнями GC; прямой C-вызов
допускает сборку при преобразовании, нативный метод сохраняет прежний барьер.

Проверены host/SWF4–8, варианты аргументов, канал, исключения, удаление при
преобразовании, GC и отказы памяти. Полные события, drop-target, точность twips,
остальные типы сцены и остальные стадии не объявляются завершёнными.
