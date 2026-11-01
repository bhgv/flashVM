# Latest-only brush fitting

[Русский](VECTOR-CANCELLATION.ru.md) · [Viewport](VECTOR-VIEWPORT.md)

Brush fitting now checks original traced contour samples against width/100 and
height/100 of the complete compound figure. The old fixed0.75-unit preliminary
simplification is removed; it could exceed the new allowance for small figures.
Input tracing is bounded to65536 samples, output to4096 commands per figure.
Oversized geometry retains raw ink with Limit. Ordinary ellipses remain four cubics.

`ideinkworker.b` supervises ordered raw acquisition and replaceable fitting.
`ideinkacquire.b` materializes each released native buffer exactly once and keeps
all prior source strokes/history. `ideinkfit.b` receives a private snapshot.
A new Job sets the old snapshot's Work cancellation capsule and immediately drops
any pending old result. It starts fitting the latest complete snapshot without
waiting for the preceding vectorization to finish. Cancelled children retire at
checkpoints during union, tracing, curve fitting and serialization; their replies
are drained but cannot publish or update the carried raw state. Stop drains them.

`idevectorcancel.b` makes cancellation transactional: Cancelled restores the exact
input shape array. Capsules are per job, monotonically cancelled and never reused.
The supervisor writes cancellation, its fitting child reads it; no window/VM
object reaches native raster workers. This is separate Dis scheduling rather than
an additional OS thread/parallel VM heap. Cancellation latency is checkpoint-bound,
not a claimed fixed millisecond limit.

Save, Undo, Open and editing selections still synchronize with the latest result.
New brush/pen input on the same capture path does not wait for old fitting.
A mixed portable Begin first adopts pending native trajectories, because its
owner-side snapshot cannot yet contain them. Input acquisition is never cancelled
as if it were obsolete geometry: doing so would lose authored strokes. Native
preview tokens retire only after a matching complete geometry publication.

Checks cover pre-cancellation, cancellation after real work starts, three successive
jobs with only latest publication, all source strokes/Undo retained, native token
Begins that overlap prior fitting, source-only SVG and Undo/Redo. A wavy contour
has an independent per-axis geometry oracle. Its32-step test flattening allows
0.15 logical units; fitting itself uses the unchanged exact1/100 sample threshold.
Reverse curve checking remains finite sampling, not a continuous Hausdorff proof.
