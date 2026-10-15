# Native drag execution / Нативное перетаскивание — v0.328

The C89 services fvm_exec_drag_begin, fvm_exec_drag_update, fvm_exec_drag_end and
fvm_exec_drag_target provide explicit context-wide native dragging without Action
codes, operand stacks or script method lookup. Each implementation has its own
file. Begin accepts a native instance, semantic center flag and optional
parent-space FVM_ExecutionDragBounds. It selects the target and captures the
world pointer without moving immediately. End flushes pending motion first.

Centered mode projects the world pointer through the inverse parent world
matrix. Delta mode applies world mouse displacement through the inverse linear
part and adds it to current local position. Bounds normalize nonfinite components
to zero and swap reversed limits. Clamped delta motion advances the remembered
mouse position only by actual world movement, preserving behavior when the mouse
overshoots and returns. Native local scale/rotation/shear are retained.

The active clip is a collector-marked context root, independent of script bindings
or pins; end/removal releases it. Removed ancestors stop the drag on update.
Input dispatch publishes its pointer then updates dragging before host callback
delivery. Queuing alone does not move. Input END does not implicitly stop dragging.
Explicit pointer set remains a position-only operation; callers may update or
end later. Geometry errors stop event delivery with the returned error; pointer
publication and script callback side effects are not rolled back.

Update roots the original clip/parent and uses the existing native geometry
collector barrier. A scoped reentry guard unwinds on every return. A revision
counter discards old transform work after a getter replaces/ends a drag; reparented
work is retried on a later update. End called from a getter cancels without
recursive flushing. Transform publication uses the existing atomic matrix setter:
allocation failures preserve position, effective previous pointer and active drag.
Singular/nonfinite parent matrices are errors, with state retained for retry.

Reference: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd, movie_clip.rs
start_drag_impl/stop_drag and player.rs update_drag. This is a bounded native
implementation: exact twip rounding, singular-parent identity fallback, drop
picking/_droptarget, all display object kinds and Flash event ordering remain
open. StartDrag/EndDrag Actions and script methods are not yet wired; the source
Action audit remains 96/100. No roadmap stage is closed.

execution_drag is C89: host/SWF4–8 transformed parents, retained clip rotation,
deferred begin/end, centered/delta modes, reversed/nonfinite bounds, overshoot,
singular-matrix retry, immediate/queued input callback ordering, removal,
independent contexts and sole drag-root survival/reclamation.
execution_drag_callbacks checks native getter exceptions, nested update rejection,
cancellation/replacement and stale-work suppression. execution_drag_allocation
injects every update/input/end allocation boundary and checks atomic position,
previous pointer, active state and restored root/native/update scopes.

## Русский

Добавлен общий C-механизм выбора, обновления, завершения и запроса drag-цели.
Он принимает явные объекты и числовые ограничения, не зависит от байткода и
стека. Цель удерживается корнем GC; начало не двигает её до обновления.
Центрирование и относительное движение учитывают трансформацию родителя,
ограничения нормализуются; при выходе мыши за границы сохраняется эффективная
предыдущая позиция. Завершение сначала выполняет отложенное перемещение.

Событие ввода обновляет drag перед host callback, очередь — только при доставке.
Удалённая цель/предок останавливает drag. Отказы памяти сохраняют матрицу и
состояние движения; отмена/замена из геттера не публикует устаревшую матрицу.
Проверены GC, исключения, вложенное обновление, вырожденный родитель и повтор
после ошибки. Это C-слой: Action StartDrag/EndDrag и скриптовые методы ещё не
подключены. Drop-target, полная точность twips и порядок событий остаются открытыми.

### v0.329 follow-up / Продолжение

StartDrag and EndDrag Action adapters now use this engine; see AVM1_DRAG.md.
Script method registration and the listed full compatibility gaps remain open.
Action-аdаптеры подключены в v0.329; скриптовые методы и остальные ограничения открыты.

### v0.330 script integration / Скриптовые методы

MovieClip.startDrag/stopDrag now share this engine through fvm_exec_drag_values;
see MOVIECLIP_DRAG.md. The original v0.328 C-only checkpoint above is historical.
Скриптовые методы подключены; прежнее описание C-only относится к точке v0.328.
