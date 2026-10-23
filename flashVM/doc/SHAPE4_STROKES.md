# Bounded SWF 8 LINESTYLE2 strokes

Current extension v0.1253+: [symmetric butt/square caps and bevel joins](STROKE_CAPS.md). Earlier round-only exclusions below are historical; miter and asymmetric caps remain unsupported.

Current extension v0.1239+: [LINESTYLE2 bitmap strokes](BITMAP_STROKES.md). Earlier blanket bitmap-stroke exclusions below are historical; other stroke limitations remain open.

[fvm_swf_read_shape](../src/swf/shape.cpp#L469) accepts DefineShape4 LINESTYLE2 records with round start/end
caps and round joins, ordinary width scaling, and optional NoClose. Width remains
an unsigned twip value; zero selects the renderer's one-pixel hairline.
Both direct RGBA colors and solid/linear/radial/focal FillType records are
accepted. Gradient fills reuse the same bounded decoder as fill banks: 1–15
ordered RGBA stops, pad/reflect/repeat, RGB/linearRGB and signed interior FIXED8.

The five reserved bits and reserved cap/join discriminator 3 return
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Other cap/join styles, NoHScale, NoVScale, PixelHinting and
bitmap stroke fills return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Focal endpoints remain unsupported.
Truncated supported records return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Unsupported style
headers can return before their remaining payload is inspected.

NoClose suppresses the automatic CLOSE command when an authored stroke returns
to its first point, leaving the ends capped. Explicit moves and style breaks
still start separate contours. The flag is compiled into owned path commands;
no extra managed flag is necessary. It does not affect fill contours.

[fvm_swf_shape_build](../src/swf/shape_graph.cpp#L117) binds gradient strokes to lineGradient instead of gradient,
keeping fill and stroke descriptors separate. The graph owns every stop and
command. Raw file bytes and backend pointers are never retained. Existing native
barriers protect construction; serialization/channel/GC use ordinary graph edges.
Decoder errors preserve the caller's previous decoded shape. A failed load
publishes no root. Existing scene rendering is not whole-scene transactional.

Stroke thickness uses the existing transformed X-axis scale and hairline policy.
Full Flash behavior under anisotropic/sheared transforms and reference raster
rounding remains open. This extension does not complete LINESTYLE2, DefineShape4,
SWF8 or any roadmap stage. Device/platform acceptance is separate.

Reference: Adobe SWF specification, LINESTYLE2, printed pages 124–125:
https://open-flash.github.io/mirrors/swf-spec-19.pdf

`tests/swf_shape4_stroke.cpp` authors independent raw LINESTYLE2 bytes, loads
actual scene characters, and compares full surfaces against the public C89
stroke/gradient APIs for 24 solid/linear/radial/focal and mode combinations.


## Validation checkpoint

Bounded LINESTYLE2 v0.1225–v0.1238: DefineShape4 now accepts round caps/joins, ordinary scaled widths, direct RGBA or solid/linear/radial/focal FillType paints, and NoClose. A shared fill decoder keeps 1–15-stop spread/interpolation/interior FIXED8 admission identical in fill and stroke banks. Managed strokes retain lineGradient rather than gradient; NoClose becomes owned command geometry and requires no new wire/resource/callback type. Thirteen new fixtures cover 24 initial type/mode cases, hairline through maximum UI16 widths, closed/capped topology, all 65536 flag headers, 180 stop-count/type/mode cases, all 511 interior FIXED8 focal positions, truncation/reserved/bitmap/focal rejection, twelve scenes copied through binary/channel transfer after sender destruction, six allocation-fault configurations, character replacement and sixteen seeks, four OS workers with 96 independent load/render/GC epochs, six mixed opposite-origin fill/stroke scenes and StateNewStyles banks. All 143 selected native tests pass; 12 ASan/UBSan tests and the allocation fixture with explicit --no-faults pass (sanitizer unwind limitation; real fault sweeps pass natively), as do five fresh-build SDK-free no-graphics admission/error tests. LeakSanitizer is disabled. Documentation source links and frontend example checks pass. Non-round caps/joins, non-scaling flags, pixel hinting, bitmap strokes, focal endpoints/clamping, exact Flash nonuniform/shear stroke behavior and reference raster rounding remain open. Physical-device/foreign-platform acceptance is separate; all nine roadmap stages remain open. [Contract](SHAPE4_STROKES.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_swf_read_shape` | function | [src/swf/shape.cpp:469](../src/swf/shape.cpp#L469) |
| `fvm_swf_shape_build` | function | [src/swf/shape_graph.cpp:117](../src/swf/shape_graph.cpp#L117) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
