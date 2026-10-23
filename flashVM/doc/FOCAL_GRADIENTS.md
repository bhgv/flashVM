# Interior focal gradients

C89 and SWF8 radial gradients support signed focal positions strictly inside the normalized unit circle, for fills and strokes, RGB/linearRGB and PAD/REFLECT/REPEAT. Zero retains the previous centered sampler exactly. The public [FVM_Gradient](../include/flashvm/gradient.h#L38) layout and existing callback identifiers remain unchanged. All nine roadmap stages remain open.

This is an interior-focal checkpoint. Adobe's API describes endpoints -1/+1 and clamping out-of-range values to them. This implementation retains [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) for those finite script inputs; it does not implement or silently approximate that endpoint policy. Full reference-player fixed-point/rounding acceptance remains open.

## C89 calls

`fvm_path_draw_gradient_focal(path, surface, gradient, even_odd, spread, interpolation, focal)` fills a path. `fvm_path_stroke_gradient_focal(path, surface, gradient, width, spread, interpolation, focal)` strokes it. Both synchronously borrow the path, description/stop array and destination; they retain no VM, root, callback or device and invoke no script or device I/O.

| Argument | Contract |
| --- | --- |
| path | Non-null borrowed editable path with existing bounded finite raster coordinates. Commands are not changed or retained. |
| surface | Non-null borrowed RGBA destination and pixel storage obeying existing dimensions/stride limits; the caller supplies exclusive access during painting. |
| gradient | Non-null [FVM_Gradient](../include/flashvm/gradient.h#L38) with its unchanged invertible bounded finite matrix, 1–15 nondecreasing stops and existing color-transform limits. Nonzero focal requires [FVM_GRADIENT_RADIAL](../include/flashvm/gradient.h#L16). |
| even_odd | Fill selector 0 or 1. The stroke entry uses existing backend stroke coverage. |
| width | Stroke-only finite positive destination-pixel width, at most [FVM_GRADIENT_MAX_STROKE_WIDTH](../include/flashvm/gradient.h#L10). Zero is invalid; use an explicit width 1 for a one-pixel stroke. |
| spread | Unsigned PAD=0, REFLECT=1 or REPEAT=2 selector. |
| interpolation | Unsigned RGB=0 or LINEAR_RGB=1 selector from [FVM_GradientInterpolation](../include/flashvm/gradient.h#L26). |
| focal | Finite normalized X position with absolute value strictly below [FVM_GRADIENT_FOCAL_LIMIT](../include/flashvm/gradient.h#L9) (1). The normalized Y position is zero. Endpoints and outside values are invalid. |

The result is [FVM_OK](../include/flashvm/flashvm.h#L42) for a successful paint. Invalid geometry, focal/type combination, selectors, width, stops or destination returns [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before painting. A valid request without a graphics backend returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Allocation failures preparing sampled colors or coverage return [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) with destination bytes intact. Atomicity covers one paint; previous successful scene paints are not rolled back when a later paint fails.

Existing gradient entries continue to use focal zero. The old no-selector fill remains PAD/RGB; spread and interpolation entries retain their previous defaults. New focal entries with focal=0 are compared byte-for-byte against the previous entries. [fvm_gradient_init](../include/flashvm/gradient.h#L48) and the public struct acquire no field.

## Geometry and numerical policy

The inverse composed gradient matrix maps each destination pixel center to normalized (x,y). For focal f, the ray starts at (f,0). Its ratio is the distance to the sample divided by the distance from the focal point to the ray's forward intersection with the unit circle. The focal point itself selects ratio zero; the unit circle selects ratio one.

Set dx=x-f, q=dx*dx+y*y, k=(1-f)*(1+f), h=sqrt(dx*dx+k*y*y), and p=f*dx. For p>=0 the ratio is (h+p)/k. For p<0 the algebraically equivalent q/(h-p) avoids subtractive cancellation. An exact focal sample q=0 returns zero. For f=0 the old sqrt(x*x+y*y) expression is used directly. Strict interior admission keeps k positive; no endpoint epsilon or invented endpoint clamp is substituted.

Spread resolves the resulting nonnegative ratio before RGB or linearRGB interpolation. Alpha remains a straight linear channel. Existing byte-domain color transforms, path/stroke coverage and source-over ordering are retained; see [linearRGB](LINEAR_RGB_GRADIENTS.md) and [stroke contracts](GRADIENT_STROKES.md).

Focal is expressed before the gradient/world affine transforms. It therefore follows the gradient axis, including rotation/reflection and nonuniform matrix transforms. Stroke width retains the existing X-axis scale/hairline policy; this extension does not establish complete Flash caps/joins/scaling or mid-path fill semantics. Stage LOW disables edge antialiasing; MEDIUM/HIGH/BEST retain the existing AA policy. Standalone C89 calls always request AA.

Math uses doubles with the existing bounded matrix/surface limits and no integer phase casts. Near-boundary focal positions and large transformed phases retain normal double-precision conditioning/phase limits; Flash Fixed8 quantization and exact reference lookup-table behavior are not claimed. No new per-pixel AVM1 metering or execution-time guarantee is added. Independent owned inputs/destinations may use separate threads; shared writes and input mutation require caller synchronization.

## AS2 and optional copied metadata

The optional eighth argument of beginGradientFill/lineGradientStyle is coerced numerically only for radial gradients. Omitted/undefined or zero retains the center. Nonzero interior values require loaded SWF8; host contexts accept them. Older loaded profiles retain [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Linear gradients ignore the argument and do not invoke its coercion callback, preserving their earlier behavior. Existing lineGradientStyle visibility and the lineStyle prerequisite are unchanged.

Finite boundary/outside values return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) without publishing a new style. Nonfinite radial coercion retains the existing successful no-op behavior. Script-side clamping to endpoint gradients remains unimplemented. Numeric coercion may invoke valueOf and have script side effects. VM/native errors and preparation allocation failures preserve the previous published style/active path unless script itself changes them; explicit effects and reentrant changes are not rolled back.

The immutable descriptor stores optional own numeric `focal` only for nonzero positions. Missing or explicit zero means centered. Restored non-number, nonfinite, boundary/outside values, or a nonzero focal paired with linear geometry, return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before one paint writes pixels. Rendering reads raw owned scalar metadata and never evaluates a focal getter or inherited property.

Caller stops/matrix/focal values are copied. duplicateMovieClip clones current and active-path aliases in one graph. Binary serialization and channels retain separate fill/stroke focal origins and color policies after sender destruction. No resource kind, wire format or callback ID is added. Receivers need the updated renderer; an older reader can ignore the optional scalar and render centered geometry instead.

All managed temporaries use ordinary graph ownership and existing native/GC barriers. The ray sampler has only local numeric state; it adds no unmanaged roots, callbacks, shared caches or asynchronous work.

## Examples and verification

The [C89 tutorial](../examples/embedding/gradients/README.md) adds an annotated focal.inc companion. It samples signed interior and centered radial ramps on a shared owned rectangle. check_gradient_focal_example.py requires fourteen measured output lines, including prior spread/linearRGB examples; CTest explicitly skips without a backend. The AS2 companion illustrates opposite fill/stroke focal positions; it is not compiled to SWF by CMake.

Tests cover known horizontal ray ratios, off-axis contour positions, both signs, near-boundary interior values, exact focal/centered cases, selectors and invalid metadata, getter exclusion, copy/duplicate/binary/channel ownership, mixed origins, coercion and allocation errors, real AVM1 interpreter/JIT calls, independent OS workers and affine/alpha/quality policy. Current build evidence appears in [compatibility status](COMPATIBILITY_STATUS.md).

## Remaining acceptance

Focal endpoints/clamping, Flash Fixed8 and original-player rounding/performance comparisons remain open. DefineShape4/LineStyle2 and on-disk focal gradient structures are not decoded here; Shape1/2/3 reserved gradient bits retain their rejection rules. Physical devices and foreign platforms remain separate. Audio/child streams, A/V/seek, text, other AS1/AS2/timeline work, GC/debugger and other JIT backends keep their independent remaining acceptance.

Primary API reference: Adobe [MovieClip beginGradientFill and lineGradientStyle](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html#beginGradientFill()).


## v0.1210 verification

Interior focal gradients v0.1197–v0.1210: additive C89 fill/stroke entries and SWF8 radial beginGradientFill/lineGradientStyle accept signed focal positions strictly inside (-1,1), with RGB/linearRGB and PAD/REFLECT/REPEAT. Zero preserves centered bytes; [FVM_Gradient](../include/flashvm/gradient.h#L38) layout, callback IDs and wire/resource types are unchanged. A cancellation-safe ray solver selects ratio before existing color/coverage composition. Finite endpoint/outside script inputs remain UNSUPPORTED; native inputs are INVALID_ARGUMENT. Validation passes 118 selected native gradient/raster/scene/graph/GC/quality/tutorial checks, ten focused ASan/UBSan checks plus explicit no-throw/no-fault runs; LeakSanitizer is disabled. Independent horizontal tables, 288 ray-contour samples and nearest-interior boundary samples pass. Real AVM1 tests cover 144 host/SWF8 interpreter/manual/automatic JIT configurations, each with success and atomic boundary rejection (288 calls), checking native counters. Four OS workers complete 128 isolated paint/GC epochs; 192 signed focal affine/alpha/quality renders pass. Copy/duplicate/current-active aliases, binary/channel sender teardown, mixed opposite origins, raw metadata/getter exclusion, numeric coercion effects and allocation fault sweeps are covered. SDK-free C89 admission/invalid-input checks pass; three graphic tutorial checks explicitly skip. A renamed detached component configures and its copied checker passes against the native executable, without a separate detached build claim. Endpoints/clamping, Flash Fixed8/reference rounding, DefineShape4/LineStyle2, complete stroke semantics and device/foreign-platform acceptance remain open. All nine roadmap stages remain open. [Contract](FOCAL_GRADIENTS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Gradient` | struct | [include/flashvm/gradient.h:38](../include/flashvm/gradient.h#L38) |
| `FVM_GRADIENT_FOCAL_LIMIT` | macro / define | [include/flashvm/gradient.h:9](../include/flashvm/gradient.h#L9) |
| `fvm_gradient_init` | function | [include/flashvm/gradient.h:48](../include/flashvm/gradient.h#L48) |
| `FVM_GRADIENT_MAX_STROKE_WIDTH` | macro / define | [include/flashvm/gradient.h:10](../include/flashvm/gradient.h#L10) |
| `FVM_GRADIENT_RADIAL` | enum constant | [include/flashvm/gradient.h:16](../include/flashvm/gradient.h#L16) |
| `FVM_GradientInterpolation` | enum | [include/flashvm/gradient.h:26](../include/flashvm/gradient.h#L26) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_path_draw_gradient_focal` | function | [include/flashvm/gradient.h:85](../include/flashvm/gradient.h#L85) |
| `fvm_path_stroke_gradient_focal` | function | [include/flashvm/gradient.h:90](../include/flashvm/gradient.h#L90) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
