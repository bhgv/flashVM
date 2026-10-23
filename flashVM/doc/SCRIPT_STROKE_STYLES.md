# ActionScript lineStyle cap and join options

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
| pixelHinting | SWF8/host Boolean conversion; false/default supported, true returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). |
| noScale | SWF8/host string conversion; normal/default supported, other modes return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). |
| capsStyle | SWF8/host string conversion; round/default, none and square supported. |
| jointStyle | SWF8/host string conversion; round/default and bevel supported; miter/unknown values return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). |
| miterLimit | Ignored for supported non-miter joins; no conversion is performed. Miter is rejected before this argument is used. |

Loaded SWF1-7 retain existing basic-method visibility and ignore all additional
arguments without invoking their conversions. Unloaded host contexts expose the
latest supported options. This does not establish correct pre-SWF6 visibility.
Optional undefined color/alpha/options retain defaults. Other option values use
counted string conversion: an embedded-NUL suffix does not match a valid keyword.
Unsupported keywords are explicit errors rather than silent round substitution.
Non-finite thickness/color/alpha inputs return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); conversion failures retain
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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
