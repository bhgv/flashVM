# Native live ink and native-sized maximization

The MovieClip desktop keeps document/history/command ownership in Limbo and AS
chrome sources. Native C prepares the painter stack, owns associated bitmaps,
composes damaged regions and submits detached raster jobs. Workers never access
VM, font, channel or document pointers. This path is independent of SWF timeline
frames.

Maximized SDI windows and MDI containers use the existing unscaled overlay parent,
physical viewport geometry and ordinary font/control metrics. Neither a saved
window zoom nor the desktop camera scales their content. Normal geometry and
camera values survive restore; changing the table zoom while maximized also keeps
native-sized content. MDI child transforms resolve the actual copied root parent.
Reparenting a container drops all borrowed child handles before a possible GC.
Show All restores normal windows before fitting the whole desktop.

## Drawing path

1. Native SDL Begin proposes a hit and a scalar capture token. The Limbo document
   admits or refuses the gesture; `inkadmit` cancels refused native captures.
2. While Brush/Pen is captured, C retains packed logical samples in its private
   buffer. It consumes at most 256 consecutive same-device motions per batch,
   retaining at most 64 batch entries. Mouse jitter merges only within one pixel
   and two milliseconds when collinear and forward-going. Turns, return segments,
   finger samples and FIFO barriers are preserved. No motion arrays or per-motion
   document requests go to Limbo.
3. C creates detached raw preview paths directly and rasterizes opaque linear ink
   with a reusable circular coverage stamp. Its solid interior uses direct RGBA
   stores; unsupported widths, curves, fills and transparency use the normal
   vector rasterizer. This is an approximate pixel preview with integer stamp
   centres, not the saved geometry. Authoring Brush and Pen semantics still differ
   in the Limbo refinement model.
4. Release freezes the native buffer and sends only its token. The separate
   `ideinkworker` process acquires the copied trajectory, builds raw geometry,
   then simplifies/vectorizes it. Same-colour touching Brush regions use the
   existing coverage-union model. Pen refines a stroked centreline after release.
5. Captured strokes queue in FIFO order; later jobs carry earlier geometry and
   history. A completed vector wire replaces its native previews only when that
   exact cumulative wire is admitted in a complete scene. Reading the buffer or
   merely finishing a worker cannot make the stroke disappear.

C also builds committed vector commands directly from validated copied geometry,
removing per-vertex AVM1 method calls from normal raster preparation. The legacy
owner-render path remains available. Unchanged vector-toolbar graphics and
immutable shelf buttons are reused. Existing per-object bitmaps, local damage,
root tiles and partial SDL texture uploads remain active; a moving nib does not
require painting every pixel of every window.

Native paint coalescing has a 4 ms budget while motion remains queued. Rendering
may be slower under load; this is a scheduling bound, not a promised display FPS.
There is no wait for a Flash frame or a raster completion before an established
native capture may collect more points. New hits and lifecycle input still obey
presentation ordering. Escape cancels active ink. Other input/lifecycle barriers
seal the stroke before that input is delivered; expose notifications only
re-present pixels. Save and geometry-changing history commands join pending jobs.
A coalescing notification process retains readiness while the controller is busy.

## C/Dis boundary

The graphical `$Idehost` builtin adds the following methods to its matching `.m`
interface. Recompile GUI Dis modules together with the updated runtime; the native
signature changes. No SWF/Dis64 file format or shared wire identifier changes.

| Method | Contract |
| --- | --- |
| `inkadmit(token, accepted)` | Scalar Begin acknowledgement; refusal retires that capture |
| `ink(token): string` | Worker-only copy of frozen eight-hex-digit packed points; empty for live/stale token |
| `inkdone(token, drawing)` | Acknowledge cumulative replacement bytes; native preview remains until matching scene commit |

Tokens are nonreused positive registry values, not C addresses. Each point is
`1001*y+x`, bounded by 1000×600. C retains at most 32 native strokes and 4096 points
per stroke; unchanged samples are suppressed. At capacity the retained prefix
survives release and the worker reports Limit. Existing document path/node bounds
still apply. Invalid acquisition cannot silently publish an incomplete Save;
its retained C preview remains and the owner reports an explicit conflict.
Closing a document retires its native previews before the slot can be reused.

`editor_ink.c/.h` owns live/frozen buffers and the publication fence;
`editor_motion_batch.c/.h` owns native FIFO batching; `editor_stamp.c/.h` is the
VM-independent raster primitive. `ideinkworker.m/.b` owns the separate Limbo
process, ordered history and coalesced notifications. The bridge in `editor_host.c`
uses generated mapped native frames and copies strings through `retstr`; no
managed pointer is stored in the native registry.

The C stamp can be called by an embedding extension with an exclusively owned
RGBA surface and immutable `FVM_Path`. It retains neither argument, allocates no
VM object and can execute on a raster worker. These private frontend headers are
source interfaces, not a newly promised shared-library ABI.

## Executable examples and checks

From a configured runtime build with the bundled font:

```sh
ctest --test-dir build/editor -R 'native_ink|maximize_native' --output-on-failure
```

`test_live_ink.c` drives 600 actual SDL motions through the production native event
owner. Only Begin and token End proposals are produced; real preview pixels at
three turns are checked. Its copied scene setup is an explicit layout boundary,
not a substitute for the separate Dis worker test.

`test_ink_runner.c` plus `ide/dis/tests/native_ink_test.b` is a commented C/Dis
handoff example: frozen native buffers go through the actual pane and worker,
SVG files and whole-stroke Undo/Redo. A nonexistent token must refuse publication.
`test_ink_primitives.c` checks live-read refusal, stale publication, capacity and
actual stamp pixels. Its timing report compares the stamp with the vector backend
without a flaky performance threshold. The optimized vector backend can be faster
for a complete isolated polyline; the main latency win here comes from removing
per-motion Dis/AVM1 exchange and rebuilding only the changed native content.
`test_maximize_native.py` compares real SDI/MDI interior RGB across saved table and
window zooms, including changes after maximize and normal restore.

See [vector editor](VECTOR-EDITOR.md), [desktop controls](dis/DESKTOP-CONTROLS.md),
[retained native scene](../disVM/runtime/RETAINED-SCENE.md) and
[Dis C modules](../disVM/runtime/NATIVE_MODULES.md).

## Pointer ownership and repeated controls (v0.1713)

The native trajectory owns the copied window/device/contact from its Begin.
Motion or release from another pointer cannot append a segment or end it.
SDL touch-generated mouse copies are discarded before lifecycle fences. If the
owning mouse reports motion without its left button after a lost release, C
freezes at the last admitted point, sends one ordinary frozen-token End, and
then consumes the hover. The hover/toolbar coordinate is never a paint endpoint.
No live point arrays are sent to Limbo by this recovery.

Mouse click counts select maximize/restore only over the actual window header.
Second and later clicks on colors, Fill and Pen remain normal control requests;
they never become window-management or drawing requests. Style changes while a
previous stroke is queued remain ordinary owner requests. The companion worker
regression now checks blue then black SVG/history across that boundary.

The real SDL regression exercises repeated toolbar clicks, foreign mouse motion
and release, lost-release hover, synthetic mouse copies and contact identity,
in addition to its original600-motion raster capture. The old editor_view body
fails the repeated-control predicate. With only the header correction it still
fails the independent no-foreign/hover-ink predicate. These are headless SDL
queue/raster and actual Dis worker gates, not physical-device or measured-FPS
acceptance. Geometry/refinement algorithms and SWF/Dis wire formats are unchanged.
