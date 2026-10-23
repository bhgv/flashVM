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
