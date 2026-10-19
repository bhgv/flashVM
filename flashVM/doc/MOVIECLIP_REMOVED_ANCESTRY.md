# Removed AVM1 ancestry / Родство удалённого клипа — v0.315

Loaded native removal and same-depth replacement now stage a retained
avm1_parent edge before clearing the physical parent. It is committed in the
same copied state map as playback and the detachment marker. Repeated removal
with no physical parent preserves the original retained edge. Unloaded host
removal keeps its earlier parentless behavior and does not create this edge.

A shared intrinsic helper reads the physical parent when present and, for loaded
execution, the retained parent when physically detached. Parent/root preloads,
native _parent/_root accessors and display-path ancestry use that helper.
Absolute paths and target-or-root fallback therefore reach the original root
from a removed clip or its still-parented descendant. Root ancestry has a named
[FVM_MOVIECLIP_ROOT_MAX_ANCESTORS](../src/standard/movieclip_constants.h#L71) bound independent from prototype lookup.

[GC:] The retained edge is an ordinary managed object value in the native private
state, reached through resource_result. Generic graph marking and transport
therefore retain/remap it without a new graph schema. A pinned removed wrapper
can keep its original root reachable; dropping the root's separate pin must not
invalidate that ancestry. Physical child tables and ordinary parent bindings
still remove the detached instance. Native storage helpers used for removal
eligibility continue reading physical parent edges, so effective detachment is
not confused with historical AVM1 provenance.

movieclip_removed_ancestry covers host/SWF4–8, same-depth replacement, repeated
removal, descendant roots, absolute sibling paths, invalid selected-target
fallback to the retained root, GC without a separate root pin, and channel
transport to a separately loaded context. movieclip_remove_allocation now also
checks the committed retained-parent edge; failed preparation still preserves
field counts, marker and both ownership sides. The existing complete Action
allocation fixture checks coherent state across later interpreter failures.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/display_object/container.rs remove_child_directly retains AVM1 parents
while clearing AVM2 parents; core/src/display_object.rs avm1_parent/avm1_root use
that ancestry. This implements native provenance only: unload/pending-removal
ordering, _lockroot, all removed-object operations and complete display lifetime
remain open. Legacy already-detached graphs without a saved edge cannot recover
a former parent. All roadmap stages remain open.

Загруженный клип сохраняет прежнего AVM1-родителя при физическом удалении или
замене. Связь фиксируется атомарно и переживает повторное удаление. `_parent`,
`_root`, preloads и абсолютные пути используют её; host-режим сохраняет прежнее
поведение. Проверены SWF4–8, потомки удалённого клипа, возврат к корню при
недействительной выбранной цели, GC, перенос через канал и отказы памяти.
События unload, отложенное удаление и полная историческая модель ещё не завершены.

Validation: Linux AGG/Cairo each passed 611 tests. Five AGG and three Cairo
executable-permission launch failures passed after restoring permissions. Four
targeted heap/global ASan+UBSan fixtures passed (no stack/leak coverage). All 26
accepted original SWFs matched the pinned oracle in each profile. The bounded
Action-reference audit remains 95/100.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_MOVIECLIP_ROOT_MAX_ANCESTORS` | macro / define | [src/standard/movieclip_constants.h:71](../src/standard/movieclip_constants.h#L71) |
<!-- source-links:end -->
