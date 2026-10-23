# Gradient strokes

FlashVM adds destination-space gradient strokes and a bounded Flash 8 `MovieClip.lineGradientStyle` implementation. Linear and centered radial ramps support PAD, REFLECT and REPEAT with ordinary RGB interpolation. This extends the existing drawing model; all nine roadmap stages remain open.

## C89 stroke entry

`fvm_path_stroke_gradient(path, surface, gradient, width, spread)` samples the existing [`FVM_Gradient`](../include/flashvm/gradient.h#L37) and composites it through a prepared backend stroke mask. The struct layout is unchanged. [`fvm_path_draw_gradient`](../include/flashvm/gradient.h#L51) and [`fvm_path_draw_gradient_spread`](../include/flashvm/gradient.h#L56) keep their fill semantics and standalone antialiasing defaults.

| Argument | Contract |
| --- | --- |
| path | Non-null borrowed editable vector path. Geometry must satisfy the existing path/raster coordinate limits. Commands are not changed or retained. Open contours are stroked as open contours. |
| surface | Non-null borrowed destination RGBA surface and pixel storage, satisfying raster dimensions/stride limits. The caller retains storage and supplies exclusive access for the call. |
| gradient | Non-null borrowed description with an invertible bounded finite matrix and 1–15 nondecreasing stops. The stop array remains valid until return. Color multipliers/additions follow the existing gradient limits. |
| width | Finite positive width in destination pixels, at most [`FVM_GRADIENT_MAX_STROKE_WIDTH`](../include/flashvm/gradient.h#L9) (4096). Zero does not request a hairline: use width 1 explicitly. |
| spread | Unsigned PAD=0, REFLECT=1 or REPEAT=2 selector from [`FVM_GradientSpread`](../include/flashvm/gradient.h#L18). Other unsigned values, including UINT_MAX, are rejected. |

RGB and alpha interpolate as straight channels. Spread normalizes the ratio before interpolation; inherited color multiplication/addition follows interpolation. Backend coverage then multiplies alpha before source-over composition. Existing caps/joins and backend-specific raster rounding are inherited from the ordinary solid stroke implementation. Sampling is at destination pixel centers; this is not a reference-player byte-for-byte guarantee.

The result is [FVM_OK](../include/flashvm/flashvm.h#L42) on successful painting. Invalid width, selector, geometry, stops, matrix or destination returns [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before painting. A valid request without a graphics backend returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Source or coverage preparation failure returns [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) and preserves destination pixels. Source samples and the complete coverage mask are prepared before composition. One stroke call is transactional; an entire scene may already contain earlier successful paints when a later paint fails.

This synchronous C89 call retains no VM, GC root, callback or device. It invokes no script and performs no device I/O. Independent owned paths/descriptions/destinations can be used on separate threads; the caller must avoid concurrent mutation of borrowed inputs and simultaneous writes to the same destination.

## AS2 lineGradientStyle

The Flash 8 method is installed as a nonenumerable versioned MovieClip method. Loaded SWF1–7 property lookup does not expose it; unloaded host contexts do. Its callback is appended to the stable method table without renumbering existing callbacks. Updated receivers are required to restore graphs containing the new native callback.

Call `lineStyle` first to enable a stroke and establish its width. `lineGradientStyle` never enables a missing/disabled stroke. It accepts the existing beginGradientFill argument model: fill type, color/alpha/ratio arrays, copied box or affine matrix, optional pad/reflect/repeat, ordinary RGB interpolation, and zero focal position for radial ramps. Counted option strings must match completely. Unsupported spread/interpolation and nonzero radial focal options return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Existing malformed-array/matrix/type no-op behavior is retained. No arguments or undefined type leaves the stroke style unchanged.

The gradient supplies stroke colors and per-stop alpha; the earlier lineStyle color/alpha do not multiply them. MovieClip/ancestor color transforms still apply. Width, fill/bitmap state and completed path descriptors are retained. Future path records retain a separate `lineGradient` edge. Style changes start a fresh path at the current drawing position using the existing path-segmentation policy. `endFill` disables only the fill and retains the selected stroke ramp. A later lineStyle replaces the current stroke ramp with a solid/disabled style. `clear` drops paths, disables strokes and releases the current descriptor.

The world matrix transforms the ramp and geometry. Stroke width follows the existing solid-stroke X-axis scale policy; a zero-width authored line uses the existing one-destination-pixel hairline. Full Flash cap/join, nonuniform scale and continuous-fill semantics across arbitrary mid-path style changes are not newly established. Stage LOW requests non-antialiased coverage; MEDIUM, HIGH and BEST use the existing antialiased backend policy. The standalone C89 entry always requests antialiasing.

## Copied state, failures and GC

Stop bytes, ratios and matrix fields are copied; the caller's arrays and matrix are not borrowed after style selection. Ordinary RGB records retain the optional spread scalar described in [gradient spread](GRADIENT_SPREAD.md). `lineGradient` is a graph edge, not a new resource kind or wire format. Missing spread defaults to PAD. Non-number, nonintegral, negative, NaN/infinite and out-of-range metadata is rejected before one stroke paints pixels.

Current ramp and active-path ramp aliases are serialized together during duplicateMovieClip. Completed paths retain their original ramps. Binary serialization and channels copy the same graph; receivers continue to paint after the sending context is destroyed. Geometry edits and current style changes in a duplicate do not mutate its source. Collection follows ordinary graph edges under the native call barrier, with no new unmanaged roots or raster-time script callbacks.

Argument coercion may run script and change managed state. Coercion exceptions and preparation allocation failures do not publish a partially built ramp or active-path replacement. Explicit script side effects are retained, including reentrant changes; the operation does not promise to roll them back. The new immutable descriptor and drawing-state map are prepared before publication. Rendering preserves existing native/GC depth cleanup.

## Examples and verification

The [C89 tutorial](../examples/embedding/gradients/README.md) owns one path, two stops and a pixel buffer. Its annotated `stroke.inc` helper prints actual red samples at ramp phases 0.5, 1 and 2. `check_gradient_strokes_example.py` checks those samples and the unchanged legacy fill. The corresponding CTest explicitly skips without a backend. The AS2 companion illustrates the required lineStyle/lineGradientStyle order; it is not compiled by the CMake tutorial checker.

Tests include an independent ordinary-stroke mask comparison, versioned availability, copied inputs, reset transitions, duplicate aliases, binary/channel ownership, malformed metadata, coercion exceptions, allocation failure sweeps, affine/alpha/quality rendering and independent OS workers. Real AVM1 CallMethod fixtures compare interpreter/manual/automatic JIT execution and verify native entry counters. Current build results are recorded in [compatibility status](COMPATIBILITY_STATUS.md).

## Remaining acceptance

DefineShape4/LineStyle2 decoding, linearRGB, nonzero focal gradients, full lineStyle caps/joins/scaling semantics and reference-player comparisons remain open. Shape1/2/3 reserved gradient bits retain their existing rejection rules. Audio, child streams, A/V synchronization, seeking, text, other AS1/AS2 APIs, timeline, GC/debugger and JIT backends keep their independent remaining work. Host regression/sanitizer checks do not certify physical devices or other platforms.

Primary API reference: Adobe's [AS2 MovieClip reference, lineGradientStyle](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html#lineGradientStyle()). File-tag restrictions: [SWF file format specification](https://open-flash.github.io/mirrors/swf-spec-19.pdf).


## v0.1182 verification

Gradient strokes v0.1169–v0.1182: additive C89 [fvm_path_stroke_gradient](../include/flashvm/gradient.h#L63) and SWF8 lineGradientStyle support linear/centered-radial PAD/REFLECT/REPEAT ordinary-RGB ramps. Existing [FVM_Gradient](../include/flashvm/gradient.h#L37) layout and callback IDs are retained; the new method callback is appended. Copied current/active stroke aliases, solid/clear/endFill transitions, duplicate and binary/channel ownership are covered. Validation: 91 selected native gradient/raster/scene/graph/GC/quality/tutorial tests pass, plus the independent mixed fill/stroke binary-transfer fixture. Ten focused ASan/UBSan checks and explicit no-throw/no-fault runs pass with LeakSanitizer disabled. Real AVM1 exercises 72 host/SWF8 interpreter/manual/automatic JIT combinations with native entry counts; four OS workers complete 128 isolated paint/GC epochs. Quality/affine/alpha tests cover 48 renders. SDK-free C89 validation passes and the graphic tutorial explicitly skips. A renamed detached component configures and its copied checker passes against the native executable; no separate detached build is claimed. Preparation fault sweeps preserve the old style/active path or destination pixels, without rolling back coercion side effects. DefineShape4/LineStyle2, linearRGB, nonzero focal, complete line caps/joins/scaling/mid-path fill semantics, original-player and physical/foreign-platform acceptance remain open. All nine roadmap stages remain open. [Contract](GRADIENT_STROKES.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Gradient` | struct | [include/flashvm/gradient.h:37](../include/flashvm/gradient.h#L37) |
| `FVM_GRADIENT_MAX_STROKE_WIDTH` | macro / define | [include/flashvm/gradient.h:9](../include/flashvm/gradient.h#L9) |
| `FVM_GradientSpread` | enum | [include/flashvm/gradient.h:18](../include/flashvm/gradient.h#L18) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_path_draw_gradient` | function | [include/flashvm/gradient.h:51](../include/flashvm/gradient.h#L51) |
| `fvm_path_draw_gradient_spread` | function | [include/flashvm/gradient.h:56](../include/flashvm/gradient.h#L56) |
| `fvm_path_stroke_gradient` | function | [include/flashvm/gradient.h:63](../include/flashvm/gradient.h#L63) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
