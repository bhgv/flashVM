# Native root locks / Блокировка корня клипа — v0.317

MovieClip now exposes the native _lockroot Boolean accessor, nonenumerable and
nondeletable on its prototype. Instances default to false; the class prototype
itself has no display flag and reads undefined. Assignments use versioned truth
conversion: loaded pre-SWF7 strings use numeric truth, SWF7+ strings use length,
and objects are true without valueOf/toString callbacks. No version visibility
mask is imposed, matching the pinned native property declaration.

The C89 [fvm_exec_clip_lock_root](../include/flashvm/execution.h#L245)(context, write, clip, input, out) reads or writes
that intrinsic state using an explicit native instance. Read ignores input;
write requires a valid value, supports input/output aliasing, and publishes a
copied private-state map only after preparation succeeds. Errors preserve stored
state and caller output. It reads no Action record, operand stack or activation.
Named uppercase macros describe its root layout and native bridge mode selectors.

[GC:] Assignment retains instance, original input and converted Boolean through
preparation. The final scalar state swap invokes no callbacks or allocation.
The flag lives in ordinary native private state; generic marking and graph
transport retain it without a new graph schema. Native getter/setter callbacks
have new stable IDs 1006/1007, leaving all previous IDs and method numbering intact.
Older graphs without the private field read unlocked. Older binaries need not
understand the new callback IDs; backward execution on older binaries is not claimed.

Intrinsic _root/preload lookup and absolute display-path root selection now stop
at the first locked ancestor. _parent is unchanged, including retained AVM1
parent edges after loaded detachment. Removing a locked subtree does not erase
its flag; unlocking it reconnects root lookup to the retained original ancestry.
The no-lock display identity/target-string implementation is unchanged.

Files keep concerns separate: clip_lock_root.cpp contains the common C service;
movieclip_root_locked.cpp reads raw intrinsic state; getter and setter native
bridges each occupy their own file. No instruction-format dependencies enter
these interfaces, so generated code can call the same C service.

Tests: execution_clip_lock_root is C89 and covers host/SWF4–8, default/alias/error
behavior, string truth, native accessors, prototype visibility and property flags.
movieclip_lockroot_paths checks root vs parent, conflicting absolute paths,
callback-free object truth, removed ancestry and native accessor/flag channel
transport. execution_clip_lock_root_allocation injects every preparation failure
for both false-to-true and true-to-false writes with aliased output and GC between
attempts. Failed writes preserve both previous flag and original input.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/globals/movie_clip.rs _lockroot declaration and lock_root/set_lock_root;
core/src/display_object.rs avm1_root. The pinned movieclip_lockroot original test
requires SWF15, asynchronous loadMovie and child.swf, so it was not used as SWF≤8
acceptance evidence. Load/unload inheritance, hitTest coordinates under root locks,
all historical display semantics and every roadmap stage remain open.

Клип получил нативный `_lockroot` и общий C89-интерфейс чтения/записи флага.
Поиск `_root` и абсолютных путей останавливается на заблокированном предке;
`_parent` не меняется. Проверены host/SWF4–8, правила истинности строк, отсутствие
пользовательских преобразований для объектов, сохранённое родство удалённых
клипов, канал и атомарность при отказах памяти. Интерфейс принимает явные значения
и пригоден для будущего JIT. Полная загрузка/выгрузка клипов и стадии роудмапа
ещё не завершены.

Validation after workspace recovery: fresh GCC 13.3 Release AGG/Cairo builds each
passed 616 tests directly. Three targeted heap/global ASan+UBSan tests passed;
stack instrumentation and LeakSanitizer remain disabled because of the known
restricted-runtime exception/stack-discovery limitation, independently reprobed.
Four original pinned assets (add_swf5, array_trivial, equals2_swf5, lessthan2_swf7)
were recovered unchanged and matched exact output with empty stderr in each
profile using --acknowledge-quit --timeline 1. The earlier 26-asset workspace was
pruned; its complete historical corpus was not rerun in this checkpoint.
The bounded Action-reference audit remains 95/100.

Follow-up v0.318: the loaded numeric hitTest point path now explicitly uses
unlocked root coordinates, independently of _lockroot. See
MOVIECLIP_HIT_ROOT_COORDINATES.md for bounded geometric evidence and remaining gaps.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_clip_lock_root` | function | [include/flashvm/execution.h:245](../include/flashvm/execution.h#L245) |
<!-- source-links:end -->
