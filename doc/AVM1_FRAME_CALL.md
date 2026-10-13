# Local frame Call / Вызов действий локального кадра

v0.108 implements ActionCall (0x9e, zero-length payload). A numeric operand
wraps to Uint32; a nonnumeric operand is string-coerced and resolves a numeric
frame string or counted label. Missing, zero and out-of-range frames do nothing.
Execution runs all selected DoAction blocks in order with the clip as receiver,
without moving the playhead, changing play state, applying scene controls,
running onEnterFrame or advancing children. A non-clip receiver uses `_root`.
Without an available MovieClip root execution remains unsupported. Paths to
another clip (colon/slash syntax) return FVM_UNSUPPORTED until target resolution
is implemented; old-version case-insensitive labels remain unverified.

[GC:] A scoped snapshot roots every selected action string and receiver across
script collection, even if the first block detaches the original segment table.
Frame recursion shares the interpreter instruction budget, is bounded at the
existing 64-activation script limit and restores debug function/offset and roots
on success, errors and allocation failure.

Вызов исполняет только действия выбранного кадра, не перемещая playhead и не
применяя его display list. Метки сравниваются целиком, а отсутствующий кадр не
вызывает ошибку. Другие пути target пока явно не поддерживаются. Snapshot
защищает блоки от GC при изменении исходного кадра первым блоком.

avm1_frame_call tests numeric/label selection, wrapping, root fallback, unchanged
playhead, missing frames, malformed operands, recursive failure/recovery and
actual script collection after detaching imported frame segments. All 269 core
AGG/Cairo tests plus the AGG runner test passed; three focused tests passed in
each headless portable and ASan/UBSan profile (existing sanitizer limits apply).
The original broad coercion-order SWF passes Call and now stops at GetURL2;
its output prefix does not count as a passing whole reference test.
