# Selected straight-stroke bounds

Current monotone axis commands: [contract](AXIS_CURVE_BOUNDS.md). Earlier blanket non-round curve rejection below now has this exception.

Current round/round curves: [analytic bounds](ROUND_CURVE_BOUNDS.md). Earlier blanket curve fallback descriptions below are historical for the entirely round style.

Current round/hairline correction: [contract](ROUND_STROKE_BOUNDS.md). The separate legacy round-only policy described below is historical.

Supported script lineStyle cap/join options are now connected in [SCRIPT_STROKE_STYLES.md](SCRIPT_STROKE_STYLES.md). Earlier file/native checkpoints below do not imply complete extended AS2 semantics.

MovieClip.getBounds and the geometry consumers that share its traversal now use
cap/join geometry for straight paths with a non-round cap or a bevel join. Butt
ends no longer gain a round-cap extension; diagonal square ends include their
actual corner extents. Root dimensions and the non-shape point hitTest use the
same updated bounds. getRect still measures the underlying path without stroke
expansion. Solid, linear/focal-gradient and bitmap paints share this geometry;
paint alpha and bitmap pixels are not read to calculate geometric bounds.

## Private geometry contract

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L503) borrows an immutable path, a positive finite width
up to 1,000,000 destination units, supported cap/join selectors and two required
caller-owned output pointers. The path's existing coordinate limits still apply.
It allocates no temporary geometry, needs no context or graphics backend and
must not race path mutation. Output bounds and has_segments are written together
only after a successful walk. Invalid arguments return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45);
quadratic or cubic commands return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) and leave both outputs intact.
An empty path or only zero-length edges succeeds with zero bounds and no segments.

The walker shares contour/tangent handling with [fvm_path_hit_test_style](../src/capi/path_hit_c.cpp#L421). It
unions finite rectangular bodies, open-end cap extrema and outer round sectors.
Only cardinal directions inside a round sector can add an axis extremum beyond
its endpoints. Bevel vertices already belong to the adjacent bodies. Closed
contours join the last and first tangents; separate Move contours finish their
own caps. NoClose remains an open contour even when endpoints coincide. A lone
Move does not enlarge a stroke's bounds.

For supported straight contours these are geometric extents subject to floating
point arithmetic, not the pixel rectangle of antialias coverage. Public C89 ABI,
public [fvm_path_bounds](../include/flashvm/path.h#L71), path drawing and point-hit behavior are unchanged. No
new serialization field, callback or resource identity is needed.

## Managed integration and explicit fallback

The shared stroke_selectors validates own lineCap/lineJoin scalars without
getters. Missing selectors preserve round defaults. The new selected path is
used when the cap is not round or the join is bevel; legacy round-only graphs
retain their previous bounds behavior, including their old hairline policy.
Selected hairlines use one destination unit. A completely collapsed selected
stroke has no extent; a fill on the same paint retains its underlying geometry.
A paint that contains both fill and stroke unions the fill-path bounds with the
selected stroke bounds. Empty stroke contours do not erase other paints/children.

Curves deliberately use a conservative fallback instead of chord bounds that
could underestimate a curve. Butt/round ends use the previous half-width axis
expansion; square ends use half-width times sqrt(2), covering diagonal cap corners.
This is not an exact curved-stroke envelope. getRect remains unchanged.
Bounds and hit queries retain the existing maximum-axis width scale, while the
renderer still uses its existing X-axis scale. Nonuniform/shear Flash metrics
and target-space stroke transformation rules remain separate acceptance work.

The existing native barrier and copied geometry/display snapshots retain graph
edges across callbacks and collection. Allocation failures in managed construction
propagate through the native call boundary; the private straight walker itself
allocates nothing. Binary/channel reconstruction, character replacement and
seek all query the currently retained graph after sender destruction or GC.
Each independent worker owns its query state.

## Remaining acceptance

Exact curved envelopes, the legacy round hairline discrepancy, nonuniform/shear
metrics, miter/asymmetric caps, hinting and non-scaling styles remain open, as do
full independent Flash reference comparison, Cairo and physical-device/platform
acceptance. The series does not complete full SWF <= 8 support or any of the nine
roadmap stages. The examples keep their existing public round APIs; the selected
file-style contract is exercised by regression fixtures.

## Fixture index

| Fixture | Scope |
| --- | --- |
| [path_style_bounds_caps](../tests/path_style_bounds_caps.cpp) | Horizontal/vertical cap extents |
| [path_style_bounds_diagonal](../tests/path_style_bounds_diagonal.cpp) | Independent 3-4-5 equations, four directions and five widths |
| [path_style_bounds_turns](../tests/path_style_bounds_turns.cpp) | Round-sector extrema, bevel and short edges |
| [path_style_bounds_contours](../tests/path_style_bounds_contours.cpp) | Closed seams, multiple contours and empty strokes |
| [path_style_bounds_errors](../tests/path_style_bounds_errors.cpp) | Transactional invalid/curve rejection |
| [swf_stroke_bounds](../tests/swf_stroke_bounds.cpp) | 24 paint/style cases, getRect and GC |
| [swf_stroke_bounds_widths](../tests/swf_stroke_bounds_widths.cpp) | 120 width/style cases with selected hairlines |
| [swf_stroke_bounds_curves](../tests/swf_stroke_bounds_curves.cpp) | Twelve conservative curve envelopes |
| [swf_stroke_bounds_metadata](../tests/swf_stroke_bounds_metadata.cpp) | Restored selectors/width errors and round defaults |
| [swf_stroke_bounds_transforms](../tests/swf_stroke_bounds_transforms.cpp) | 96 uniform transforms and 24 complete collapses |
| [swf_stroke_bounds_mixed](../tests/swf_stroke_bounds_mixed.cpp) | Sixteen stroke-only/fill/isolated-Move/degenerate graphs |
| [swf_stroke_bounds_hitbox](../tests/swf_stroke_bounds_hitbox.cpp) | 24 root dimension read/write and non-shape point-hit cases |
| [swf_stroke_bounds_quality](../tests/swf_stroke_bounds_quality.cpp) | 96 independent stage-quality cases |
| [swf_stroke_bounds_transfer](../tests/swf_stroke_bounds_transfer.cpp) | 24 graphs through binary/channel after sender teardown |
| [swf_stroke_bounds_timeline](../tests/swf_stroke_bounds_timeline.cpp) | Replacement and sixteen alternating seeks |
| [swf_stroke_bounds_threads](../tests/swf_stroke_bounds_threads.cpp) | Four OS workers, 96 bounds/GC epochs |
| [swf_stroke_bounds_allocation](../tests/swf_stroke_bounds_allocation.cpp) | 24 managed getBounds C++ allocation-fault sweeps |


## Checkpoint v0.1303

Selected stroke bounds v0.1285–v0.1303: straight paths with a non-round cap or bevel join now measure finite bodies, cap corners and outer round-sector extrema instead of expanding every path box as a round stroke. getBounds, root dimensions and non-shape point hits share the result; getRect stays stroke-free. Empty selected strokes and isolated Moves do not create extents; mixed fill/stroke paints retain the union. The private allocation-free walker rejects curves transactionally. Managed curves use an explicit conservative fallback, widened by sqrt(2) for square caps. Seventeen new fixtures cover analytic axial and independent diagonal equations, outer sectors and short edges, contour closure, empty geometry, transactional errors, 24 SWF paint/style cases, 120 width cases, twelve curve envelopes, restored metadata, 96 uniform transforms plus 24 complete collapses, sixteen mixed/degenerate graphs, root dimension reads/writes and hit boxes, 96 stage-quality cases, 24 graphs through binary/channel after sender teardown, replacement and sixteen seeks, four workers with 96 bounds/GC epochs and 24 managed C++ allocation-fault sweeps. Validation passes 228 selected native regressions, all 17 fixtures in the SDK-free no-graphics interpreter profile, 16 ASan/UBSan fixtures plus allocation with explicit --no-faults, and documentation/example checks. Real allocation failures pass natively; sanitizer unwinding remains limited and LeakSanitizer is disabled. Public C89 ABI, wire formats and legacy round-only bounds policy stay unchanged. Exact curve bounds, round hairline discrepancies, nonuniform/shear metrics, unsupported LINESTYLE2 options, independent full Flash reference acceptance, Cairo and physical platforms remain open. All nine stages remain open. [Contract](STROKE_BOUNDS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_path_bounds` | function | [include/flashvm/path.h:71](../include/flashvm/path.h#L71) |
| `fvm_path_hit_test_style` | function | [src/capi/path_hit_c.cpp:421](../src/capi/path_hit_c.cpp#L421) |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:503](../src/capi/path_hit_c.cpp#L503) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
