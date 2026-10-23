# C89 script stroke styles

Diagonal extension: none/bevel, width=10, start (2,2), control (8,6.5), end (14,11). Segment length is 15 and normal half-width offsets are 3 and 4; bounds are -1..17, -2..15 despite moveTo(100,100). The added line is `linear curve bounds: -1..17, -2..15`. Ownership and result GC roots follow the preceding stages. [Contract](../../../doc/LINEAR_CURVE_BOUNDS.md).

Axis-curve extension: after the round example it selects none/bevel, control (8,8), end (14,8), and isolated moveTo(100,100). It verifies bounds x=2..14, y=6..10. The additional line is `axis curve bounds: 2..14, 6..10`. Each replaced getBounds result is separately rooted and released. C APIs and synchronous ownership are unchanged; no new C entry point is needed. [Contract](../../../doc/AXIS_CURVE_BOUNDS.md).

Round-curve extension: after the first render, the example clears the old line, selects round/round, calls curveTo with control (8,0), end (14,8), and adds isolated moveTo(100,100). It verifies getBounds x=0..16, y=2..10. The previous bounds root is released before replacing geometry; the new result is separately rooted and cleaned up. clear, curveTo and moveTo synchronously mutate context-owned geometry through the existing [fvm_object_call](../../../include/flashvm/value.h#L126). An additional output line is `round curve bounds: 0..16, 2..10`. This extension does not test the curve raster. [Contract](../../../doc/ROUND_CURVE_BOUNDS.md).

[Русский](README.ru.md). This executable calls script lineStyle with square/bevel,
draws (2,8)..(14,8), verifies bounds 0..16 and a corner hit at (0.1,6.1).
With AGG it also checks alpha 255 at pixel (0,6). It needs no SWF compiler,
device, SDL or event loop.

Parameter contract and limits: [SCRIPT_STROKE_STYLES.md](../../../doc/SCRIPT_STROKE_STYLES.md).

## Ownership and execution

The context owns the clip and temporary bounds; the host pins both across VM
calls. Numeric values are copied; static strings are borrowed only for synchronous
calls. The stack RGBA buffer belongs to the host and render does not retain it.
Cleanup drops only acquired roots, then destroys context and runtime. Ordinary
errors return nonzero without assertions. Every nonempty C line is explained.

## Build and run

From the standalone FlashVM component:

```sh
cmake -S . -B build-stroke-styles -DFVM_DEFS_FILE="$PWD/cmake/profiles/text-jit.defs"
cmake --build build-stroke-styles --target flashvm_examples_embedding_stroke_styles_main
./build-stroke-styles/flashvm_examples_embedding_stroke_styles_main
```

```text
bounds: 0..16; square corner: hit
square corner alpha: 255
round curve bounds: 0..16, 2..10
axis curve bounds: 2..14, 6..10
linear curve bounds: -1..17, -2..15
```

With `cmake/profiles/minimal.defs`, use a separate build directory: the second
line becomes `renderer unavailable; geometry verified`, still with exit status 0.
An unloaded host context enables the latest supported options; SWF1-7 ignore
additional arguments. This example does not certify miter, hinting, non-scaling,
Cairo, nonuniform transforms or full Flash reference compatibility.

## Demonstrated C APIs

| Function | Purpose, ownership and execution |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L62) | Initialize caller-owned configuration; no retained pointers. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L64) | Create the owned runtime synchronously; destroy it after all contexts. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L68) | Create an isolated owned context whose objects remain context-owned. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L20) | Synchronously install built-in script methods in this context. |
| [fvm_movieclip_create](../../../include/flashvm/movieclip.h#L95) | Create a context-owned clip; root it before later VM work. |
| [fvm_gc_pin](../../../include/flashvm/gc.h#L48) | Acquire one host GC root for a borrowed clip or bounds object. |
| [fvm_value_undefined](../../../include/flashvm/value.h#L40) | Initialize or reset a caller-owned value; no owned allocation. |
| [fvm_value_number](../../../include/flashvm/value.h#L42) | Copy a scalar operand into caller-owned storage. |
| [fvm_value_boolean](../../../include/flashvm/value.h#L44) | Copy a Boolean operand without retaining any object. |
| [fvm_value_string](../../../include/flashvm/value.h#L46) | Borrow a static literal for the synchronous method call; keep it alive. |
| [fvm_object_call](../../../include/flashvm/value.h#L126) | Synchronously coerce/copy lineStyle, append geometry, query hitTest or allocate getBounds; arguments are borrowed and returned objects are context-owned. |
| [fvm_object_get](../../../include/flashvm/value.h#L72) | Read a copied numeric bounds property while its owner is pinned; getters may execute. |
| [fvm_movieclip_render](../../../include/flashvm/movieclip.h#L135) | Synchronously borrow the host surface; report UNSUPPORTED without a renderer. |
| [fvm_gc_unpin](../../../include/flashvm/gc.h#L54) | Release exactly one acquired root; never access the object after context teardown. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) | Finalize all context-owned clips and temporary results after dropping roots. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | Release the runtime only after destroying its context. |

[main.c](main.c) · [Catalog](../../README.md#example-embedding-stroke_styles).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:95](../../../include/flashvm/movieclip.h#L95) |
| `fvm_movieclip_render` | function | [include/flashvm/movieclip.h:135](../../../include/flashvm/movieclip.h#L135) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../../../include/flashvm/value.h#L72) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `fvm_value_boolean` | function | [include/flashvm/value.h:44](../../../include/flashvm/value.h#L44) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
| `fvm_value_string` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
