# SWF 8 bitmap LINESTYLE2 strokes

DefineShape4 LINESTYLE2 FillType records now accept bitmap modes 0x40–0x43.
They retain the existing repeat/edge-extension and nearest/premultiplied-bilinear
sampling policies, RGBA source alpha, matrix composition, and stage edge quality.
Round caps/joins, ordinary scaled width, hairlines and NoClose use the same path
commands as solid and gradient strokes. Special caps/joins, pixel hinting and
non-scaling flags remain unsupported under the LINESTYLE2 contract.

[fvm_swf_read_shape](../src/swf/shape.cpp#L467) validates and owns the style; the scene loader requires a
previously defined bitmap character. Missing/zero IDs, malformed matrices,
truncation and old SWF versions return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Decoder failure
retains the caller's previous result; failed scene loading publishes no root.

[fvm_swf_shape_build](../src/swf/shape_graph.cpp#L117) binds stroke images under lineBitmap and fill images under
bitmap, retaining the dictionary's existing BitmapData identity. These descriptors
own scalar matrix/flags and a traced source edge, never a backend pointer. Generic
binary/channel transfer retains bitmap resources and aliases. The native render
barrier protects borrowed pixels; contexts remain subject to existing thread rules.
No public C89 layout, callback index, wire tag or resource type changes.

[fvm_movieclip_bitmap_draw](../src/standard/bitmap_fill.cpp#L156) chooses lineBitmap for positive destination stroke width.
Disposed images contribute no pixels; live changes appear in subsequent renders.
Malformed owned metadata returns [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before destination writes.

The private [fvm_path_bitmap_paint_quality](../src/capi/bitmap_paint_c.cpp#L155) helper accepts width zero for a fill or
a finite positive width up to [FVM_BITMAP_STROKE_MAX_WIDTH](../src/render/quality_internal.hpp#L43) for a round stroke.
It borrows path, surfaces and paint only during the synchronous call; it invokes
no script or host callback. It validates before backend selection, so valid
NO_GRAPHICS calls return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) and invalid calls return
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). All source samples and coverage are prepared before
composition, permitting source/destination aliasing. Allocation failures return
[FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) and preserve one paint's destination. Rendering a whole scene
is not one transaction. Existing public [fvm_path_draw_bitmap](../include/flashvm/bitmap_paint.h#L26) keeps width zero
and its previous ABI/bytes.

## Verification and remaining acceptance

Raw-byte fixtures exercise actual scene loading and full surfaces. The oracle
samples a filled rectangle and separately rasterizes a solid stroke, combining
coverage with source alpha; independent center pixels check expected red/blue
and alpha for repeat versus edge extension. Tests cover all four bitmap modes,
20 width/mode cases, 16 open/closed/NoClose cases, 20 subpixel sampling cases,
16 stage-quality cases, sender teardown, live/disposed pixels, malformed metadata,
source aliasing, bitmap ID/version admission, allocation fault sweeps, seeks,
four OS workers, independent fill/stroke images, and new style banks.

Flash reference raster/rounding, exact anisotropic/sheared stroke behavior,
remaining LINESTYLE2 flags, device/foreign-platform acceptance and all nine
roadmap stages remain open. This is a bounded file-rendering extension, not
complete SWF8 support or a new script drawing API.

Reference: Adobe SWF specification, LINESTYLE2 and bitmap FILLSTYLE:
https://open-flash.github.io/mirrors/swf-spec-19.pdf


## Validation checkpoint

Bitmap LINESTYLE2 v0.1239–v0.1252: DefineShape4 accepts all four bitmap FillType modes for bounded round strokes, retaining repeat/edge-extension, nearest/premultiplied-bilinear sampling, live/disposed RGBA images, hairlines, NoClose and stage edge quality. Stroke descriptors retain lineBitmap separately from bitmap fills and preserve dictionary BitmapData identity across GC and binary/channel transfer. The shared private sampler stages every source pixel and coverage before composition, including aliased surfaces, without changing public C89 layouts/functions, callback IDs or resource/wire tags. Thirteen new fixtures cover independent known-color pixels and full-surface sampling/coverage oracles, twenty width/mode cases, sixteen open/closed/NoClose cases, twenty shifted sampling cases, sixteen quality cases, four binary/channel scene transfers after sender teardown, malformed IDs/versions/widths/metadata, live mutation/dispose, four decoder/loader/paint C++ allocation-fault configurations, sixteen seeks, four OS workers with 96 independent load/render/GC epochs, four mixed distinct-image fill/stroke scenes and four StateNewStyles banks. Validation passes 157 selected native tests including both bitmap-fill and gradient C89 tutorials, 12 ASan/UBSan tests plus explicit --no-faults allocation execution, and five fresh-build SDK-free no-graphics tests. Actual C++ allocation failures pass natively; sanitizer unwinding is limited and LeakSanitizer is disabled. Source links and frontend example checks pass. Non-round caps/joins, non-scaling/hinting flags, exact anisotropic/sheared stroke behavior, Flash reference raster/rounding and physical-device/foreign-platform acceptance remain open. All nine roadmap stages remain open. [Contract](BITMAP_STROKES.md).

[modes](../tests/swf_bitmap_stroke.cpp), [widths](../tests/swf_bitmap_stroke_widths.cpp), [close](../tests/swf_bitmap_stroke_close.cpp), [sampling](../tests/swf_bitmap_stroke_sampling.cpp), [quality](../tests/swf_bitmap_stroke_quality.cpp), [transfer](../tests/swf_bitmap_stroke_transfer.cpp), [errors](../tests/swf_bitmap_stroke_errors.cpp), [metadata](../tests/swf_bitmap_stroke_metadata.cpp), [allocation](../tests/swf_bitmap_stroke_allocation.cpp), [timeline](../tests/swf_bitmap_stroke_timeline.cpp), [threads](../tests/swf_bitmap_stroke_threads.cpp), [mixed](../tests/swf_bitmap_stroke_mixed.cpp), [styles](../tests/swf_bitmap_stroke_styles.cpp).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_BITMAP_STROKE_MAX_WIDTH` | macro / define | [src/render/quality_internal.hpp:43](../src/render/quality_internal.hpp#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_bitmap_draw` | function | [src/standard/bitmap_fill.cpp:156](../src/standard/bitmap_fill.cpp#L156) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_path_bitmap_paint_quality` | function | [src/capi/bitmap_paint_c.cpp:155](../src/capi/bitmap_paint_c.cpp#L155) |
| `fvm_path_draw_bitmap` | function | [include/flashvm/bitmap_paint.h:26](../include/flashvm/bitmap_paint.h#L26) |
| `fvm_swf_read_shape` | function | [src/swf/shape.cpp:467](../src/swf/shape.cpp#L467) |
| `fvm_swf_shape_build` | function | [src/swf/shape_graph.cpp:117](../src/swf/shape_graph.cpp#L117) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
