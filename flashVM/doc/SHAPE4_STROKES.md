# Bounded SWF 8 LINESTYLE2 strokes

[fvm_swf_read_shape](../src/swf/shape.cpp#L470) accepts DefineShape4 LINESTYLE2 records with round start/end
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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_swf_read_shape` | function | [src/swf/shape.cpp:470](../src/swf/shape.cpp#L470) |
| `fvm_swf_shape_build` | function | [src/swf/shape_graph.cpp:117](../src/swf/shape_graph.cpp#L117) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
