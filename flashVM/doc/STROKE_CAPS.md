# Symmetric LINESTYLE2 caps and bevel joins

Supported script lineStyle cap/join options are now connected in [SCRIPT_STROKE_STYLES.md](SCRIPT_STROKE_STYLES.md). Earlier file/native checkpoints below do not imply complete extended AS2 semantics.

Selected straight-stroke bounds are now described in [STROKE_BOUNDS.md](STROKE_BOUNDS.md). Exact curves and nonuniform/shear metrics remain open; this earlier checkpoint does not certify them.

Selected geometric hit testing is now covered by [STROKE_HIT_TEST.md](STROKE_HIT_TEST.md); exact bounds and nonuniform/shear metrics remain open. The original cap-rendering checkpoint below remains historical.

DefineShape4 now renders matching round, butt (no extension), or square start/end
caps, combined with round or bevel joins, for solid, linear/focal gradient and
bitmap strokes. NoClose continues to leave coincident endpoints capped.
Different start/end caps, miter joins, pixel hinting and non-scaling flags return
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Reserved cap/join values and reserved bits return
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before unsupported-style classification. Truncated supported
records remain invalid, and decoder failure preserves the previous result.

[fvm_swf_read_shape](../src/swf/shape.cpp#L469) copies the two style selectors into owned paint metadata.
[fvm_swf_shape_build](../src/swf/shape_graph.cpp#L117) publishes optional lineCap and lineJoin scalar properties;
missing properties retain round defaults in old graphs. The renderer reads own
numeric integral metadata without getters. Invalid, fractional, non-finite or
out-of-range selectors return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before that stroke writes any
pixels. Entire scenes remain non-transactional.

Private per-call selectors in [fvm_path_draw_quality](../src/capi/path_c.cpp#L316),
[fvm_path_mask_width_quality](../src/capi/path_c.cpp#L499), [fvm_path_gradient_focal_quality](../src/capi/gradient_c.cpp#L240) and
[fvm_path_bitmap_paint_quality](../src/capi/bitmap_paint_c.cpp#L155) propagate the same geometry into solid colors,
gradient/bitmap masks and BitmapData.draw coverage. They borrow all inputs for
the synchronous call, invoke no callbacks and retain no style/cache state.
Invalid selectors are rejected before backend selection; valid NO_GRAPHICS
operations return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Existing preparation/OOM ownership contracts
remain in force. Public C89 functions/layouts retain round defaults and their ABI.

The AGG backend maps to butt_cap, round_cap, square_cap and bevel_join; Cairo maps
to its matching cap/join enums. Validation in this checkpoint uses native AGG and
SDK-free no-graphics profiles; Cairo/device/reference acceptance remains separate.
There is no new AS2 drawing method or argument-coercion policy in this extension.

## Verification and remaining scope

Independent geometric checks assert exact filled rectangles for horizontal butt
and square caps, partial circular corner coverage for round caps, and a clipped
bevel corner versus a round corner. Full-surface tests cover 24 style/paint cases,
120 width cases, 96 topology cases, 96 quality cases, malformed scalar metadata,
round restoration, 24 binary/channel scene transfers, allocation fault sweeps,
16 seeks, four workers with 96 load/render/GC epochs, mixed fill/stroke order and
24 new style banks. The complete 65536-header LINESTYLE2 fixture is updated to
the expanded admission set.

Miter limits, asymmetric caps, hinting/non-scaling, exact bounds/hit-test behavior
for non-round styles, anisotropic/sheared Flash stroke rules, reference raster
rounding and physical-device/foreign-platform acceptance remain open. All nine
roadmap stages remain open; this checkpoint extends file rendering only.

Reference: Adobe SWF specification, LINESTYLE2:
https://open-flash.github.io/mirrors/swf-spec-19.pdf


## Validation checkpoint

Symmetric LINESTYLE2 caps/joins v0.1253–v0.1266: DefineShape4 now renders matching round/butt/square caps and round/bevel joins through solid, linear/focal-gradient and bitmap stroke paints. Optional lineCap/lineJoin scalars retain style through GC and graph transfer; absent metadata retains round defaults. Shared per-call coverage selectors reach sampled masks and BitmapData.draw coverage without public C89 ABI or callback/resource/wire changes. Reserved fields remain invalid; asymmetric caps, miter and scaling/hinting flags remain explicitly unsupported. Thirteen new fixtures include independent rectangle/circular/bevel geometry checks, 24 initial paint/style cases, 120 width cases, 96 topology cases, 96 quality cases, native selector/truncation/unsupported rejection, malformed metadata and round restoration, 24 scenes through binary and channel transfer after sender destruction, 24 decoder/loader/paint C++ allocation-fault configurations, sixteen frame seeks, four OS workers completing 96 independent load/render/GC epochs, six mixed fill/stroke scenes and 24 StateNewStyles banks. The full 65536-header admission fixture passes the expanded policy. Validation passes 170 selected native AGG tests, 12 ASan/UBSan tests plus the allocation fixture with explicit --no-faults, and five SDK-free no-graphics tests from a new build directory. Actual allocation fault sweeps pass natively; sanitizer unwinding is limited and LeakSanitizer is disabled. Documentation/source links and frontend example checks pass. Miter limits, asymmetric caps, scaling/hinting, exact styled bounds/hit tests, anisotropic/sheared Flash rules, Cairo/reference raster and physical-device/foreign-platform acceptance remain open. All nine roadmap stages remain open. [Contract](STROKE_CAPS.md).

[paints](../tests/swf_stroke_caps.cpp), [analytic](../tests/swf_stroke_caps_analytic.cpp), [widths](../tests/swf_stroke_caps_widths.cpp), [close](../tests/swf_stroke_caps_close.cpp), [quality](../tests/swf_stroke_caps_quality.cpp), [errors](../tests/swf_stroke_caps_errors.cpp), [metadata](../tests/swf_stroke_caps_metadata.cpp), [transfer](../tests/swf_stroke_caps_transfer.cpp), [allocation](../tests/swf_stroke_caps_allocation.cpp), [timeline](../tests/swf_stroke_caps_timeline.cpp), [threads](../tests/swf_stroke_caps_threads.cpp), [mixed](../tests/swf_stroke_caps_mixed.cpp), [styles](../tests/swf_stroke_caps_styles.cpp).


Use a new CMake build directory after this internal C++ signature/layout change; stale test objects from an older build can reference the prior private renderer signatures.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_path_bitmap_paint_quality` | function | [src/capi/bitmap_paint_c.cpp:155](../src/capi/bitmap_paint_c.cpp#L155) |
| `fvm_path_draw_quality` | function | [src/capi/path_c.cpp:316](../src/capi/path_c.cpp#L316) |
| `fvm_path_gradient_focal_quality` | function | [src/capi/gradient_c.cpp:240](../src/capi/gradient_c.cpp#L240) |
| `fvm_path_mask_width_quality` | function | [src/capi/path_c.cpp:499](../src/capi/path_c.cpp#L499) |
| `fvm_swf_read_shape` | function | [src/swf/shape.cpp:469](../src/swf/shape.cpp#L469) |
| `fvm_swf_shape_build` | function | [src/swf/shape_graph.cpp:117](../src/swf/shape_graph.cpp#L117) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
