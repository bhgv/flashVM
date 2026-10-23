# Linear-light RGB gradients

Current file extension v0.1211+: [fill-only SWF8 DefineShape4](SHAPE4_FILLS.md) adds 15-stop modes and signed interior FIXED8 focal records. LINESTYLE2 and focal endpoints remain unsupported. Earlier blanket Shape4 exclusions below are historical. All nine stages remain open.


Interior focal extension v0.1197+: [signed radial fill/stroke origins](FOCAL_GRADIENTS.md) / [внутренние focal-позиции](FOCAL_GRADIENTS.ru.md). Earlier zero-focal-only notes describe historical checkpoints. Endpoints/clamping, DefineShape4/LineStyle2 and all nine roadmap stages remain open.


Explicit linearRGB interpolation is available for C89 gradient fills/strokes and the Flash 8 beginGradientFill/lineGradientStyle methods. The existing [FVM_Gradient](../include/flashvm/gradient.h#L38) layout, old entry-point defaults and callback identifiers remain unchanged. All nine roadmap stages remain open.

## C89 entries

`fvm_path_draw_gradient_interpolated(path, surface, gradient, even_odd, spread, interpolation)` fills a vector path. `fvm_path_stroke_gradient_interpolated(path, surface, gradient, width, spread, interpolation)` strokes it with existing backend coverage. Both borrow the path, gradient/stop array and destination for one synchronous call. They retain no VM, root, callback or device and invoke no script or device I/O.

| Parameter | Contract |
| --- | --- |
| path | Non-null borrowed path; finite coordinates obey the existing raster/path limits. Commands are not changed. |
| surface | Non-null RGBA destination and borrowed storage satisfying existing dimensions/stride limits. The caller provides exclusive access during painting. |
| gradient | Non-null [FVM_Gradient](../include/flashvm/gradient.h#L38) with its existing bounded finite invertible matrix, sorted 1–15 stops and color-transform limits. Layout and ownership are unchanged. |
| even_odd | Fill selector 0 or 1. The stroke entry uses the existing nonzero stroke mask. |
| width | Stroke-only finite positive destination-pixel width, at most [FVM_GRADIENT_MAX_STROKE_WIDTH](../include/flashvm/gradient.h#L10). Zero is invalid; an explicit width 1 requests a one-pixel stroke. |
| spread | Unsigned PAD=0, REFLECT=1 or REPEAT=2 selector, as in the existing spread API. |
| interpolation | Unsigned [FVM_GRADIENT_INTERPOLATION_RGB](../include/flashvm/gradient.h#L28)=0 or [FVM_GRADIENT_INTERPOLATION_LINEAR_RGB](../include/flashvm/gradient.h#L29)=1. Other values, including UINT_MAX, are invalid. |

[FVM_OK](../include/flashvm/flashvm.h#L42) indicates successful painting. Invalid selectors, width, path, surface, stops or matrix return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before painting. A valid request without a graphics backend returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Allocation failure preparing samples or coverage returns [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) and preserves destination bytes. One paint is transactional; prior successful paints in a scene are not rolled back by a later failure.

The old [fvm_path_draw_gradient](../include/flashvm/gradient.h#L52) remains PAD/RGB. [fvm_path_draw_gradient_spread](../include/flashvm/gradient.h#L57) and [fvm_path_stroke_gradient](../include/flashvm/gradient.h#L64) retain RGB for every spread selector. Explicit interpolation=RGB uses the same byte-domain sample arithmetic; full-surface tests compare it against those old entries. [fvm_gradient_init](../include/flashvm/gradient.h#L48) and the public gradient struct acquire no new field.

## Transfer and composition order

RGB mode interpolates encoded color bytes as before. linearRGB normalizes stop RGB bytes to 0–1, decodes the piecewise sRGB transfer, interpolates in linear light, then re-encodes the result. Alpha always interpolates as a straight linear channel; it is neither gamma converted nor used to premultiply the stop colors.

The selected transfer is the standard piecewise sRGB curve, not a single gamma-2.2 approximation. Decoding uses c/12.92 at c<=0.04045, otherwise ((c+0.055)/1.055)^2.4. Encoding uses 12.92*l at l<=0.0031308, otherwise 1.055*l^(1/2.4)-0.055. [W3C color conversion reference](https://www.w3.org/TR/css-color-4/#color-conversion-code).

Spread resolves the ramp position before interpolation. Duplicate-stop selection and endpoint behavior are unchanged. Endpoints and constant channels retain exact byte values before inherited transforms. After re-encoding RGB, the existing byte-domain multiply/add transform and final component rounding/clamping apply. Path coverage multiplies alpha, followed by the existing straight-alpha source-over composition.

Only the at-most-fifteen stops are decoded once per paint into stack-local data. There is no mutable global lookup table. Per-pixel encoding remains synchronous native math; no new per-pixel AVM1 work metering or execution-time guarantee is introduced. Independent owned paths, descriptions and destinations can be painted on separate threads; callers must synchronize shared writes and input mutation.

A black-to-white midpoint is 128 in ordinary RGB and 188 in this linearRGB sampler. With alpha stops 0 and 255, its alpha is 128 in both modes. These analytical host results do not certify every original-player lookup-table/rounding detail.

## ActionScript and copied records

The optional seventh argument of beginGradientFill/lineGradientStyle accepts the exact counted strings RGB and linearRGB. Omitted/undefined selects RGB. linearRGB requires loaded SWF8; host contexts without a loaded movie accept it. Loaded SWF6/7 retain [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) for this extension, with prior drawing state intact. Counted strings with embedded-NUL suffixes or unknown spelling remain unsupported. lineGradientStyle retains its existing SWF8 visibility and lineStyle prerequisite.

Both methods copy a numeric optional `interpolation` property into the immutable gradient record only for linearRGB. Missing metadata defaults to RGB; explicit 0 also selects RGB. Only exact own numeric 0 or 1 is accepted. Fractional, negative, non-number, NaN/infinite and larger values return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before one paint changes pixels. Rendering uses raw owned metadata and does not evaluate an interpolation getter or inherited property.

Current/active path aliases are copied with the same graph in duplicateMovieClip. Binary serialization and channels retain interpolation independently of the caller's arrays/matrix and the sending context's lifetime. Both fill and stroke records retain their separate policies. Existing wire/resource types and callbacks are reused. A receiver must contain the updated sampler: an older reader can ignore this new optional field and render RGB instead.

Coercion can invoke script. VM exceptions, native callback errors and preparation allocation failures do not publish a partial new descriptor or active-path replacement. Explicit script side effects, including reentrant changes, are not rolled back. Ordinary graph ownership and native/GC barriers retain temporary objects; no managed callback is invoked while raster samples are computed.

## Examples and checks

The [C89 tutorial](../examples/embedding/gradients/README.md) adds an annotated linear_rgb.inc companion. It paints the same black/white midpoint in RGB and linearRGB, for fills and strokes, and prints straight RGBA from actual destination pixels. check_gradient_linear_rgb_example.py requires all eight output lines, including the earlier fill/spread demonstration. CTest explicitly skips the backend-free executable. The AS2 companion illustrates a SWF8 linearRGB clip; it is not compiled to SWF by CMake.

Tests cover analytic signed linear/radial phases, dark transfer branches, duplicate/constant stops, alpha and source-over order, legacy RGB byte identity, versions, counted option names, copy/duplicate/binary/channel ownership, malformed metadata/getter exclusion, allocation and coercion failures, real AVM1/JIT, independent workers and stage/affine/alpha policies. Current build evidence appears in [compatibility status](COMPATIBILITY_STATUS.md).

## Remaining work

DefineShape4/LineStyle2 and their on-disk interpolation bits are not decoded by this extension. Shape1/2/3 reserved-bit rejection is unchanged. Nonzero focal gradients, full Flash stroke caps/joins/scaling and mid-path fill semantics remain open. Original-player rounding/performance comparisons, physical devices and foreign platforms require separate validation. Audio, child streams, A/V/seek, text, other AS1/AS2/timeline functions, GC/debugger and other JIT backend acceptance retain their existing remaining work.

Primary API reference: Adobe [MovieClip beginGradientFill and lineGradientStyle](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html#beginGradientFill()).


## v0.1196 verification

Linear-light RGB v0.1183–v0.1196: additive C89 fill/stroke interpolation selectors and SWF8 beginGradientFill/lineGradientStyle support linearRGB for linear and centered-radial PAD/REFLECT/REPEAT ramps. [FVM_Gradient](../include/flashvm/gradient.h#L38) layout, old RGB defaults and callback IDs remain unchanged. The sampler decodes at most fifteen sRGB stops into paint-local storage, interpolates RGB in linear light, re-encodes before color transforms and keeps alpha straight/linear. Endpoints and constant channels remain exact. Validation passes 105 selected native gradient/raster/scene/graph/GC/quality/tutorial tests, ten focused ASan/UBSan checks plus explicit no-throw/no-fault runs; LeakSanitizer is disabled. Independent numeric tables cover dark transfer branches, duplicate stops, alpha/source-over and old RGB byte identity. Seventy-two real host/SWF8 AVM1 interpreter/manual/automatic JIT combinations check native counters; four OS workers complete 128 isolated paint/GC epochs; 96 affine/alpha/quality renders pass. Mixed RGB/linearRGB fill/stroke binary graphs retain independent policies after sender teardown. SDK-free C89 validation passes and two graphic tutorial checks explicitly skip. A renamed detached component configures and its copied checker passes against the native executable; no separate detached build is claimed. Counted names, version gates, copied inputs, duplicate/current-active aliases, raw metadata/getter exclusion and preparation fault sweeps are covered. DefineShape4/LineStyle2, nonzero focal, complete stroke scaling/caps/joins/mid-path fill semantics, per-pixel work metering and original-player/device/foreign-platform acceptance remain open. All nine roadmap stages remain open. [Contract](LINEAR_RGB_GRADIENTS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Gradient` | struct | [include/flashvm/gradient.h:38](../include/flashvm/gradient.h#L38) |
| `fvm_gradient_init` | function | [include/flashvm/gradient.h:48](../include/flashvm/gradient.h#L48) |
| `FVM_GRADIENT_INTERPOLATION_LINEAR_RGB` | enum constant | [include/flashvm/gradient.h:29](../include/flashvm/gradient.h#L29) |
| `FVM_GRADIENT_INTERPOLATION_RGB` | enum constant | [include/flashvm/gradient.h:28](../include/flashvm/gradient.h#L28) |
| `FVM_GRADIENT_MAX_STROKE_WIDTH` | macro / define | [include/flashvm/gradient.h:10](../include/flashvm/gradient.h#L10) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_path_draw_gradient` | function | [include/flashvm/gradient.h:52](../include/flashvm/gradient.h#L52) |
| `fvm_path_draw_gradient_interpolated` | function | [include/flashvm/gradient.h:71](../include/flashvm/gradient.h#L71) |
| `fvm_path_draw_gradient_spread` | function | [include/flashvm/gradient.h:57](../include/flashvm/gradient.h#L57) |
| `fvm_path_stroke_gradient` | function | [include/flashvm/gradient.h:64](../include/flashvm/gradient.h#L64) |
| `fvm_path_stroke_gradient_interpolated` | function | [include/flashvm/gradient.h:77](../include/flashvm/gradient.h#L77) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
