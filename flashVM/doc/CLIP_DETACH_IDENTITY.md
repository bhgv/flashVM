# Native detach identity / Признак отсоединения — v0.312

New native MovieClip state records a private numeric detachment marker, initially
false. Forced native removal and displacement by another clip at the same depth
stage a true marker in the same copied property map as the cleared parent and
stopped playback. No marker is published before the existing transaction commits;
allocation failure must preserve the original marker and ownership graph.

The C89 [fvm_exec_clip_detached](../include/flashvm/execution.h#L329)(context, clip, out) returns this instance's marker
as an [FVM_Value](../include/flashvm/value.h#L26) boolean. It bypasses public properties, accepts native instances
only, rejects foreign/plain/prototype objects and preserves output on error.
Legacy private state without the marker reads false; malformed marker types or
numbers are rejected. Named uppercase macros document the two stored states.
No public structure layout or graph schema version changes. This query invokes
no script or collection; temporary native lookup allocation errors are contained.

A parentless fresh root returns false, while a detached wrapper returns true.
Removing an already displaced wrapper cannot mark or remove its replacement.
This supplies an explicit native identity fact for future removal/target logic;
it does not yet implement historical AVM1 removed state. In particular, markers
are not propagated to descendants, unload events are not introduced, and
activation target fallback does not consult the marker. RemoveSprite remains
absent until its depth, target and lifecycle policy is implemented. Existing
forced removal still accepts authored negative depths and parentless roots.

execution_clip_detached is a C89 fixture covering host/SWF4–8, fresh roots,
attached/displaced/detached instances, stale removal, public-property shadowing,
foreign/plain validation and GC. execution_clip_detached_legacy checks old and
malformed private state with preserved error output. movieclip_remove_allocation
now verifies the marker remains false at every failed removal allocation and
becomes true only on success, alongside existing ownership checks.

В приватном состоянии клипа появился отдельный признак отсоединения. Он отличает
новый корневой клип от удалённого экземпляра с таким же пустым parent. Удаление и
замена на той же глубине записывают признак в рамках существующей атомарной
операции; отказ памяти сохраняет прежнее значение. Общая C-функция читает его
без доступа к байткоду, стеку или переопределяемым свойствам. Проверены старое и
повреждённое состояние, SWF4–8, C89, GC и отказы выделения памяти.
Распространение состояния на потомков, события unload и правила RemoveSprite
ещё предстоит реализовать; все стадии роудмапа остаются открытыми.

Validation: Linux AGG/Cairo each passed 603 tests. AGG had 21 executable-permission
launch failures; all passed after restoring permissions. Cairo completed directly.
Three targeted heap/global ASan+UBSan tests passed (no stack/leak instrumentation).
All 26 accepted original SWFs matched the pinned oracle in each profile.
Action-reference coverage remains 94/100; no absent Action was claimed implemented.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_clip_detached` | function | [include/flashvm/execution.h:329](../include/flashvm/execution.h#L329) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
