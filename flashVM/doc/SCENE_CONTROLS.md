# Captured scene controls — v0.634

[Русский](SCENE_CONTROLS.ru.md). During [`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L66), authored SWF
placement controls can invoke registered constructors. A constructor previously
could replace a later list slot, change its scalar metadata, clear the list or
append a new placement into the live traversal. The current frame now captures
its semantic control fields before its first placement is applied.

Each frame list is a dense zero-based ordinal sequence, independent of insertion
order, with at most [`FVM_MOVIECLIP_MAX_SCENE_CONTROLS`](../src/standard/movieclip_constants.h#L25) entries. Missing, null,
nonobject or foreign-context control edges reject the local capture with
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) before any control in that list is applied. Semantic errors
inside a structurally valid control can still fail after earlier controls commit.
The bound concerns control records, separately from frame action/initializer limits.

The snapshot copies op/depth/token/character/flags/background and optional symbol,
matrix, color, ratio, name and clip_actions values. Unknown properties are ignored.
Replacement of a list slot or a captured record's scalar field affects a later
scene evaluation, never this local batch. Appended controls are deferred until
that frame is evaluated again; ordinary later ticks need not revisit its controls.
The source list and host mutations remain changed after return. Shared resource
objects retain their identity: mutation *inside* a captured symbol/matrix/color/
clip-actions object is not frozen by this shallow snapshot. There is no new public
metadata-editing API or whole-movie immutable snapshot.

Seek/loop reconstruction uses the same local capture per visited authored frame,
then composes placements without executing skipped frame actions. Earlier display
changes, constructor effects and background selections survive later failures.
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) during capture applies none of that local list, but does not
roll back earlier frames or change the existing timeline failure/retry policy.

[GC:] The existing native scene/tick barrier retains copied managed values and
strings after source deletion. Collection inside a constructor remains rejected;
collection after the host call succeeds. Native callback userdata remains borrowed
host memory and must outlive invocation. No C ABI or serialized format changes.

Regression tests: `scene_controls_snapshot` (SWF6–8, interpreter and actual native
JIT, all four mutations and shuffled insertion), `scene_controls_invalid`
(structural holes/types/owners/cap), `scene_controls_allocation` (allocation sweep,
cleanup and GC). Existing scene/frame/initializer tests remain relevant. The
[C89 tick tutorial](../examples/embedding/movieclip/README.md) demonstrates owner-thread
advancement and committed work; private mutation fixtures are tests, not public API
examples. Run from the component root:

```sh
cmake -S . -B build-scene -DCMAKE_BUILD_TYPE=Release
cmake --build build-scene --target flashvm_tests_scene_controls_snapshot flashvm_tests_scene_controls_invalid flashvm_tests_scene_controls_allocation
ctest --test-dir build-scene -R '^flashvm_tests_scene_controls_' --output-on-failure
```

All nine roadmap stages remain open. These tests compare FlashVM behavior and do
not establish original-Player lifecycle rules or foreign-platform/device acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `FVM_MOVIECLIP_MAX_SCENE_CONTROLS` | macro / define | [src/standard/movieclip_constants.h:25](../src/standard/movieclip_constants.h#L25) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
