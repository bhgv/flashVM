# SWF 8 DefineShape4 fills

The bounded decoder [fvm_swf_read_shape](../src/swf/shape.cpp#L432) now accepts fill-only DefineShape4 (tag 83).
The scene loader gates it to SWF 8 and later. DefineShape/2/3 retain their original
reserved-mode checks and limit of eight gradient stops.

The supported gradient records use 1–15 ordered RGBA stops, pad/reflect/repeat,
RGB/linearRGB, and linear, radial or focal radial fills. A focal value is decoded
from signed little-endian FIXED8 without rounding through a script conversion.
Only the strict interior (-1, 1) is supported; endpoints and values beyond them
return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Reserved gradient modes and malformed/truncated records
return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Equal ratios are permitted.

ShapeBounds and EdgeBounds are validated for ordering and coordinate bounds.
The fill-only layout retains ShapeBounds. SWF 8 reserved header bits, including
the winding-rule bit introduced in SWF 10, must be zero. The two stroke hint bits
are accepted without changing fill rendering. Every nonempty LINESTYLE2 bank
returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46), including banks added by StateNewStyles. Caps, joins,
stroke fills and scaling require a later implementation.

Solid and bitmap fills reuse the existing shape paths and bitmap dictionary
validation. Each gradient becomes an owned scalar-only managed descriptor through
[fvm_swf_shape_build](../src/swf/shape_graph.cpp#L117), using optional spread/interpolation/focal properties.
No backend pointer or source byte storage survives decoding. The loader's native
barrier protects construction; normal graph edges retain stops and commands.
Decoder failure leaves the caller's previous decoded shape intact.

The native sampler's existing rounding, transform, quality and allocation limits
apply. This is a bounded extension, not full DefineShape4 or SWF 8 acceptance.
Device validation, Flash reference raster comparisons and all roadmap stages
remain open.

Reference: Adobe SWF specification, Shapes and Gradients, pages 132–136:
https://open-flash.github.io/mirrors/swf-spec-19.pdf

## Verification

`tests/swf_shape4.cpp` authors raw tag bytes and checks decoded metadata and
complete rendered surfaces against the public focal-gradient sampler for 18
linear/radial/focal × spread × interpolation cases. The sampler itself has
independent analytic tests in `gradient_focal_edges.c` and `gradient_linear_rgb.c`.


## Additional fixtures

`swf_shape4_errors` covers truncation and unchanged previous output;
`swf_shape4_stops` covers 90 stop-count/mode combinations;
`swf_shape4_fixed` covers all 511 interior FIXED8 values;
`swf_shape4_transfer` covers 12 scenes through binary/channel copies after sender destruction;
`swf_shape4_version` covers version gates, duplicate IDs and sprite boundaries;
`swf_shape4_allocation` sweeps six decoder/loader configurations through allocation failures;
`swf_shape4_styles` covers new RGBA banks and LINESTYLE2 rejection;
`swf_shape4_bounds` covers EdgeBounds and stroke hints;
`swf_shape4_bitmap` covers four bitmap modes and missing resources;
`swf_shape4_timeline` covers character replacement and 16 seeks;
`swf_shape4_threads` covers four workers and 96 load/render/GC epochs.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_swf_read_shape` | function | [src/swf/shape.cpp:432](../src/swf/shape.cpp#L432) |
| `fvm_swf_shape_build` | function | [src/swf/shape_graph.cpp:117](../src/swf/shape_graph.cpp#L117) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
