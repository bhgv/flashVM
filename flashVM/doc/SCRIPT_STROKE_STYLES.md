# ActionScript lineStyle cap and join options

Current round/round curves: [analytic bounds](ROUND_CURVE_BOUNDS.md). Earlier blanket curve fallback descriptions below are historical for the entirely round style.

Current round/hairline correction: [contract](ROUND_STROKE_BOUNDS.md). The separate legacy round-only policy described below is historical.

[MovieClip.lineStyle](../src/standard/movieclip.cpp#L1438) now connects supported Flash Player 8 option strings to the
same selected stroke geometry used by restored LINESTYLE2 records. It supports
round, none (butt) and square caps and round/bevel joins on solid and linear,
radial or interior focal-gradient strokes. The reference signature is Adobe's
[AS2 MovieClip.lineStyle](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html#lineStyle()).
This is a bounded implementation, not full lineStyle or SWF <= 8 acceptance.

## Arguments and errors

| Argument | Implemented behavior |
| --- | --- |
| thickness | Missing or explicit undefined disables the current stroke without coercing later arguments. Finite numbers clamp to 0..255; zero is hairline. Existing fractional widths are preserved. |
| rgb | Missing/undefined defaults to black; finite numbers retain existing packed 24-bit conversion. |
| alpha | Missing/undefined defaults to 100; finite values clamp to 0..100. |
| pixelHinting | SWF8/host Boolean conversion; false/default supported, true returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48). |
| noScale | SWF8/host string conversion; normal/default supported, other modes return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48). |
| capsStyle | SWF8/host string conversion; round/default, none and square supported. |
| jointStyle | SWF8/host string conversion; round/default and bevel supported; miter/unknown values return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48). |
| miterLimit | Ignored for supported non-miter joins; no conversion is performed. Miter is rejected before this argument is used. |

Loaded SWF1-7 retain existing basic-method visibility and ignore all additional
arguments without invoking their conversions. Unloaded host contexts expose the
latest supported options. This does not establish correct pre-SWF6 visibility.
Optional undefined color/alpha/options retain defaults. Other option values use
counted string conversion: an embedded-NUL suffix does not match a valid keyword.
Unsupported keywords are explicit errors rather than silent round substitution.
Non-finite thickness/color/alpha inputs return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47); conversion failures retain
their original status. Unknown/coercion/non-finite Flash edge behavior remains
separate reference acceptance.

## State, snapshots and allocation

[line_style](../src/standard/movieclip.cpp#L1438) converts width/color/alpha through [line_number](../src/standard/movieclip.cpp#L1379), then [line_options](../src/standard/movieclip.cpp#L1392)
handles Boolean hinting and scale/cap/join strings before publication, in argument
order; optional strings are compared by [line_option](../src/standard/movieclip.cpp#L1371). No supported code path
converts miterLimit. User conversion callbacks can run script and mutate state;
those deliberate side effects are not rolled back. The staged state is copied
after conversion so their latest fill/drawing state is retained.

[line_style](../src/standard/movieclip.cpp#L1438) builds a temporary property map, resets absent cap/join to round,
removes only the current lineGradient and resets the active drawing record. One
map swap publishes the new style after every allocation succeeds. Native
allocation failure or unsupported/failed conversion cannot partially replace
this current style, apart from user callback side effects. Previously completed
paint records and their gradient edges remain independent.

When drawing begins a new record, optional lineCap/lineJoin scalars are copied
from current state. A later lineStyle call starts a separate record; clear resets
the selectors. beginFill/endFill and lineGradientStyle retain the selected line
style as appropriate. Round defaults remain absent, preserving old graphs.
No public C89 ABI, callback ID or wire format changes are required. Existing
render, geometric hit and straight-bound consumers use the same scalars.

The native call barrier protects receiver, argument and graph edges while
conversion runs. Reentrant explicit collection from a native conversion callback
is rejected by the existing collector policy; collection succeeds after the outer
call restores its scopes. Binary/channel transfer retains both current and
completed styles after sender destruction. Workers use isolated contexts.

## Example and limits

The executable [C89 tutorial](../examples/embedding/stroke_styles/README.md)
verifies square-cap bounds and a corner hit, then checks an opaque corner pixel
when AGG rendering is available. NO_GRAPHICS still executes geometry checks.
The tutorial uses ordinary error handling and explicitly roots its result object.

Miter, pixel hinting, non-scaling modes, exact curved envelopes, legacy round
hairline bounds, nonuniform/shear Flash metrics, independent full player-reference
coercion/raster acceptance, Cairo and physical devices remain open. All nine
roadmap stages and complete SWF <= 8 compatibility remain open.


## Fixtures

| Fixture | Scope |
| --- | --- |
| [movieclip_script_stroke_styles](../tests/movieclip_script_stroke_styles.cpp) | Six retained solid styles and collection |
| [movieclip_script_stroke_defaults](../tests/movieclip_script_stroke_defaults.cpp) | Width clamps, undefined defaults and disable |
| [movieclip_script_stroke_versions](../tests/movieclip_script_stroke_versions.cpp) | SWF1-8 option gates |
| [movieclip_script_stroke_errors](../tests/movieclip_script_stroke_errors.cpp) | Unsupported, non-finite and embedded-NUL rejection |
| [movieclip_script_stroke_transitions](../tests/movieclip_script_stroke_transitions.cpp) | Independent records, clear and round restoration |
| [movieclip_script_stroke_gradients](../tests/movieclip_script_stroke_gradients.cpp) | Eighteen linear/radial/focal style cases |
| [movieclip_script_stroke_curves](../tests/movieclip_script_stroke_curves.cpp) | 48 quadratic and corner-path cases |
| [movieclip_script_stroke_quality](../tests/movieclip_script_stroke_quality.cpp) | 96 quality/style/paint combinations |
| [movieclip_script_stroke_bounds](../tests/movieclip_script_stroke_bounds.cpp) | Selected bounds and shape hits |
| [movieclip_script_stroke_mixed](../tests/movieclip_script_stroke_mixed.cpp) | Fill/current/path style independence |
| [movieclip_script_stroke_callbacks](../tests/movieclip_script_stroke_callbacks.cpp) | Ordered callbacks, failure recovery and fill side effects |
| [movieclip_script_stroke_allocation](../tests/movieclip_script_stroke_allocation.cpp) | Six atomic current-state C++ fault sweeps |
| [movieclip_script_stroke_duplicate](../tests/movieclip_script_stroke_duplicate.cpp) | Six independent copies of current/completed styles |
| [movieclip_script_stroke_transfer](../tests/movieclip_script_stroke_transfer.cpp) | 24 graphs through binary and channel after sender teardown |
| [movieclip_script_stroke_threads](../tests/movieclip_script_stroke_threads.cpp) | Four workers, 96 render/GC epochs |
| [movieclip_script_stroke_avm1](../tests/movieclip_script_stroke_avm1.cpp) | Eighteen real versioned AVM1 calls |
| [jit_script_stroke_styles](../tests/jit_script_stroke_styles.cpp) | 36 modes/styles with native-entry evidence |
| [movieclip_script_stroke_timeline](../tests/movieclip_script_stroke_timeline.cpp) | Frame actions and sixteen alternating seeks |

## Checkpoint v0.1324

Script lineStyle options v0.1304–v0.1324: SWF8 and unloaded host contexts now accept round/none/square caps and round/bevel joins through the existing ActionScript method. Current state publishes atomically after ordered conversion and allocation; new paths copy selectors, completed paths retain their own styles, and clear restores defaults. Missing/undefined thickness disables strokes; finite width clamps to 0..255 and optional undefined color/alpha retain defaults. SWF1-7 ignore extra arguments without conversion. Hinting, non-normal scale modes, miter and unknown options return explicit unsupported status without partial publication; non-finite width/color/alpha remain invalid. Eighteen new fixtures cover solid/gradient geometry, six widths and default/disable cases, version gates, unsupported/NUL/non-finite errors, separate records and resets, eighteen gradient styles, 48 curve/corner cases, 96 quality cases, bounds/hits, mixed fill state, six ordered callback failure stages plus native GC rejection/recovery and deliberate fill side effects, six C++ allocation-fault publication sweeps, six duplicates, 24 graphs through both binary and channel after sender teardown, four OS workers with 96 render/GC epochs, eighteen real AVM1 cases, 36 interpreter/manual/native-JIT configurations with native-entry counters and frame actions across sixteen alternating seeks. A fully commented C89 tutorial verifies bounds, a square-only corner hit and available raster alpha, with explicit roots for its result object. Validation passes 247 selected native tests including the tutorial, 18 SDK-free no-graphics tests (17 fixtures and tutorial), 17 ASan/UBSan tests (16 fixtures and tutorial) plus allocation with explicit --no-faults, and source/example checks. JIT evidence is native; real C++ fault sweeps pass natively and without graphics. Sanitizer unwind limits remain and LeakSanitizer is disabled. Public C89 ABI, callback IDs and wire formats stay unchanged. Full coercion/reference raster, pre-SWF6 method visibility, miter/hinting/non-scaling, exact curves/nonuniform metrics, Cairo and physical-platform acceptance remain open. All nine stages remain open. [Contract](SCRIPT_STROKE_STYLES.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
