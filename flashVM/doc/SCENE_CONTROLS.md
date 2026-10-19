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

## Scene work (v0.635)

[`FVM_ExecutionWorkScope`](../src/core/execution_work_scope.hpp#L9) now spans scene preparation and callbacks, inheriting
an outer AVM1/tick budget instead of resetting it per control or constructor.
Incremental delivery charges one unit per captured control and one before each
application. Capture failure changes no display control from that local list.
An absent/empty incremental list adds zero work, preserving ordinary empty ticks.

Reconstruction charges one per visited authored frame (including gaps), one per
captured control and one per composition, one per old-child copy and one per
old-child inspection, and one before each desired placement/update. For F visited
frames, K controls, M old children and D desired entries, this is F+2K+2M+D native
units, excluding nested constructors and other existing execution work. Old child
snapshots are bounded by [`FVM_MOVIECLIP_ADVANCE_MAX_CHILDREN`](../src/standard/movieclip_constants.h#L32); copying is precharged.
Depth sorting is not part of this revision. Private protocol/color/identity values
are named in [scene_constants.h](../src/standard/scene_constants.h) separately from traversal limits.

Numeric ceiling exhaustion returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) without incrementing beyond that
ceiling; saturated inherited counters fail without wrap. Zero numeric limit still
polls cooperative worker cancellation. Native callback wall time, all map operations,
resource copies and internal placement allocation sizes are not individually timed
or charged. This is logical work accounting, not a real-time bound.

Earlier placements, background selection, constructor side effects and outer frame
state remain committed on later failure. The scene-frame completion marker is
published only after success; retry can reevaluate previously applied placements.
No transactional whole-scene rollback or new lifecycle scheduler is introduced.
Exact forward/reconstruction/existing-child/gap/nested/saturated boundaries are
covered by `scene_controls_budget`; two real OS workers stop before traversal and
between native-only control applications in `scene_controls_worker`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `FVM_MOVIECLIP_ADVANCE_MAX_CHILDREN` | macro / define | [src/standard/movieclip_constants.h:32](../src/standard/movieclip_constants.h#L32) |
| `FVM_MOVIECLIP_MAX_SCENE_CONTROLS` | macro / define | [src/standard/movieclip_constants.h:25](../src/standard/movieclip_constants.h#L25) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
