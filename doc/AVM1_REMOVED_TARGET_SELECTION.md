# Selecting removed targets / Выбор удалённой цели — v0.316

Loaded fvm_exec_select_target now checks effective native detachment before
publishing a selected clip. An empty path or pre-SWF7 undefined reset cannot
reselect a detached base clip. A path resolving through an ordinary alias to a
still-parented descendant of a detached clip also becomes an invalid/null target.
A live sibling reached through the retained AVM1 ancestry remains selectable.

This uses the existing shared native ancestor helper; it does not duplicate
Action decoding, depth policy or path parsing. Both SetTarget and SetTarget2
already call this C89 service. Standalone host selection keeps its prior behavior.
Initial activation construction is unchanged: the reference initializes it with
the base clip and applies removal filtering when selecting a new target.

[GC:] Existing input/path roots remain active through the native detachment
check. The helper invokes no script/GC. A preparation failure before publication
leaves the prior selected target intact; caller callbacks may still have their
own independent side effects. No structure layout, graph schema or ABI changes.

avm1_removed_target_selection covers host/SWF4–8, encoded SetTarget empty paths,
SetTarget2 empty/missing operands, alias-based removed descendants and live sibling
paths from removed bases, with synchronous activation-state probes. The allocation
fixture injects every preparation failure for alias/nested selection and checks
that the previous target/root chain remains unchanged until successful null
selection. The retained-ancestry/channel fixture supplies regression coverage.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/activation.rs set_target_clip filters avm1_removed while constructors
initialize target_clip to Some(base_clip). Complete removed-object semantics,
unload ordering, scope equivalence and all roadmap stages remain open.

В загруженных SWF выбор новой цели теперь отбрасывает отсоединённый клип и
потомков отсоединённого дерева. Сброс на удалённый базовый клип оставляет цель
недействительной; живая соседняя цель по сохранённому пути доступна. Проверены
SWF4–8 и прежнее host-поведение, обе инструкции выбора цели, отсутствие операнда,
синхронное состояние активации и отказы памяти с сохранением предыдущей цели.

Validation: Linux AGG/Cairo each passed 613 tests. Three AGG and two Cairo
executable-permission launch failures passed after restoring permissions. Three
targeted heap/global ASan+UBSan fixtures passed (no stack/leak instrumentation).
All 26 accepted original SWFs matched the pinned oracle in both profiles.
The source Action-reference audit remains 95/100; all stages remain open.
