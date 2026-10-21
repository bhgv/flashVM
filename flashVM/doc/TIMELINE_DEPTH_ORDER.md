# Native timeline depth snapshots — v0.629

[Русский](TIMELINE_DEPTH_ORDER.ru.md). [`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L116) now actually visits
children in ascending native depth order. Earlier tick documentation promised
that order, but the copied native hash table iterated in insertion order. This
revision corrects the traversal and checks pending descendant detachment.

## Local snapshot and ordering contract

For each nonstatic native clip, retain the existing sequence: frame/scene actions,
retained EnterFrame records, optional onEnterFrame method, then prepare children.
Root precedes descendants. At each parent, sort native depth keys ascending and
visit each captured child depth-first. Signed public depths share the parent's
stored bias, so negative depths correctly precede nonnegative ones. Legacy
nonnegative slot tables retain their native ordering policy.

Preparation is local to one parent, after its callbacks. A child created by that
parent's handler may run in the same tick. Once prepared, identities and depth
order remain fixed: a sibling callback's newborn or same-depth replacement waits
for the next parent snapshot. Moving an existing child within the same parent does
not reorder its current turn. The next tick sees the updated order. A child's
own newly prepared subtree can still observe changes made before that child's turn.
There is no whole-tree snapshot or once-per-global-tick registry.

The snapshot copies values, not just keys followed by live-table lookup. This
prevents a replacement at a captured key from impersonating the former receiver.
Before child callbacks, validate the complete local table: each value is a same-
context native MovieClip with same-context private state, no repeated identity
or self-child. Malformed/foreign/nonclip/alias entries return INVALID_ARGUMENT.
The parent may already have executed; preparation failure does not roll it back.
Native raw parent changes are rechecked at each turn, so moved/detached children skip.

Immediately before each eligible child entry, query effective native removal,
including its ancestors. Removing a branch in its own onEnterFrame therefore
suppresses its pending descendants even when their physical parent edges still
point to that removed branch. This is not an onUnload scheduler. Explicitly
selecting an already removed root retains the previous direct-root policy; it does
not grant its removed descendants normal scene delivery. Removal after a receiver
has begun does not roll back that receiver's actions.

## Shared work and bounds

[depth_order.hpp](../src/core/depth_order.hpp) provides one C++98 heap sort for both
this timeline path and the retained scene-input paths. No STL container or quadratic
insertion sorting is introduced. Every sift comparison block and extraction charges
the enclosing execution work, with unchanged sort accounting for existing input APIs.
Timeline child entries are precharged before snapshot allocation, as before; each
actual child eligibility query now charges an additional unit. AVM1, parent methods,
nested calls and multiple requested ticks still share the original outer budget.
A childless single-frame tick remains two units. Zero ticks still does no work.

Each post-handler child table is capped at [`FVM_MOVIECLIP_ADVANCE_MAX_CHILDREN`](../src/standard/movieclip_constants.h#L32)
(65,536 slots), including static artwork and stale links. Oversized tables fail
before scanning/copying that table. Existing maximum descendant depth and ticks per
call remain. There is no new total-node cap across a complete multi-tick call.
The native removal query itself uses its existing bounded ancestry walk; one
charged query is not a count of every property/hash operation inside that helper.

Numeric exhaustion or a cooperatively polled worker stop returns ERROR. Snapshot
allocation returns OUT_OF_MEMORY. Actual callbacks retain their error/exception
results. Parent/earlier-child effects survive later failure; remaining children
and ticks skip. A future outer call starts with committed state and a fresh budget,
without a hidden resume cursor. Native callbacks and allocations are not preempted.

[GC:] The existing outer native barrier holds copied raw receiver identities and
code graphs alive across removals/replacements. Collection remains invalid inside
the call; incremental step collects nothing. Snapshot vectors, local alias tables
and work scopes are stack-local and are not serialized. Ordinary scene/function
serialization carries the native depths and methods needed to reconstruct ordering.
Each visited node also verifies its native/context state before following it.

Initial frame/child-container edges are separately checked for null and context
ownership before parent callbacks. Those early failures have no receiver callback
side effects; malformed individual slots are checked later at snapshot preparation.

## Evidence and example

- [Depth order](../tests/movieclip_tick_depth_order.cpp): host policy and SWF5–8,
  shuffled insertion, negative depths, root-first DFS and repeated ticks.
- [Identity](../tests/movieclip_tick_snapshot_identity.cpp): same-depth replacement,
  depth mutation during a sibling callback and next-tick reordered delivery.
- [Removal](../tests/movieclip_tick_detached_branch.cpp): a detached ancestor with
  still-linked descendants cannot deliver their pending handlers.
- [Invalid tables](../tests/movieclip_tick_snapshot_invalid.cpp) and
  [bounds](../tests/movieclip_tick_snapshot_bound.cpp): malformed values, foreign
  contexts, aliases, self/nonclip links and oversized local snapshots before children.
- [Private edges](../tests/movieclip_tick_private_edges.cpp): null/foreign containers
  and wrong-owner native resource state fail without unsafe dereference.
- [Budget](../tests/movieclip_tick_depth_budget.cpp),
  [OOM](../tests/movieclip_tick_depth_allocation.cpp), existing tick workers:
  sorting exhaustion, partial ordered callbacks, allocation sweeps and guard recovery.
- [Shared sort](../tests/depth_order.cpp): odd/even sizes, all small rotations,
  duplicates and exact work-limit boundaries without a library sort oracle.
- [AVM1 graph/JIT](../tests/movieclip_tick_depth_graph.cpp): root decimal trace 123
  after shuffled child insertion, sender destruction, traced-string GC and actual
  interpreter/native execution in SWF5–8 contexts.

A separate source-linked control built with the v0.628 movieclip.cpp produced
`root, high, low, middle`; the new source produced `root, low, middle, high` for the
same host graph. This proves the old insertion-order defect, not original-player
conformance. The [strict C89 tutorial](../examples/embedding/movieclip/README.md)
keeps the five-unit example and adds an independent finite-budget runtime printing
`depth order: low middle high` after shuffled construction.

The existing clone-allocation regression covers ten version/entry-route combinations
and thousands of allocation boundaries. Its 30-second default timed out in the
instrumented run, so CTest now gives that exhaustive test the same bounded 180-second
allowance as other exhaustive allocation matrices. Assertions and sweep coverage
are unchanged. Restored cached pre-v0.628 object files were invalidated before the
final regression run; mixing those objects with the current private context layout
is not a valid runtime result.

A small independent component-root reproduction needs no SDL or device:

```sh
cmake -S . -B build-depth -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-depth --target flashvm_tests_depth_order flashvm_tests_movieclip_tick_depth_order flashvm_examples_embedding_movieclip_main
ctest --test-dir build-depth -R '^(flashvm_tests_depth_order|flashvm_tests_movieclip_tick_depth_order|flashvm_examples_embedding_movieclip_main)$' --output-on-failure
```

This is Linux core/timeline evidence. Raster child order, general lifecycle/hit
scheduling, original-player equivalence, physical devices and foreign-platform
acceptance are separate. All nine roadmap stages remain open.


Verification for this revision: 172 selected Release CTests and 112 whole-core
ASan/UBSan CTests passed on Linux; the final sanitizer suite ran serially, with leak
detection disabled. Its clone-allocation matrix completed in 81.61 seconds. The
strict C89 example also passed against the instrumented core. [Stage status and
acceptance limits](ROADMAP_STATUS.md).

v0.630: raster traversal now has a separate [EN](RASTER_DEPTH_ORDER.md) / [RU](RASTER_DEPTH_ORDER.ru.md) contract; v0.629 evidence above remains historical.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:116](../include/flashvm/movieclip.h#L116) |
| `FVM_MOVIECLIP_ADVANCE_MAX_CHILDREN` | macro / define | [src/standard/movieclip_constants.h:32](../src/standard/movieclip_constants.h#L32) |
<!-- source-links:end -->
