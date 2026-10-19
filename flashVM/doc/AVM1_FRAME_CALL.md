# Local frame Call / Вызов действий локального кадра

v0.108 implements ActionCall (0x9e, zero-length payload). A numeric operand
wraps to Uint32; a nonnumeric operand is string-coerced and resolves a numeric
frame string or counted label. Missing, zero and out-of-range frames do nothing.
Execution runs all selected DoAction blocks in order with the clip as receiver,
without moving the playhead, changing play state, applying scene controls,
running onEnterFrame or advancing children. A non-clip receiver uses `_root`.
Without an available MovieClip root execution remains unsupported. Paths to
another clip (colon/slash syntax) return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) until target resolution
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

## v0.111 — Target-prefixed frame calls

Counted `clip-path:frame` operands now resolve through the display path helper
before action selection. Child, absolute and parent prefixes and embedded NUL
are tested. Missing targets are silent no-ops. Frames still execute actions only.
Прежнее ограничение на все пути снято для формы `путь:кадр`; без разделяющего
двоеточия альтернативная slash-форма пока не реализована.


## v0.631 frame block preparation / Подготовка блоков кадра

Tick/ActionCall capture a bounded dense zero-based action list by ordinal, share
native work limits and preserve selected code through source mutation. Public native
frame calls include operand coercion; labels choose the lowest stored exact match.
[EN](FRAME_BLOCKS.md) / [RU](FRAME_BLOCKS.ru.md). Это не закрывает стадии 1–9.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
