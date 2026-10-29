# Event-driven background rendering

[Русский](RENDER_ASYNC.ru.md) · [Workspace](../../ide/DESKTOP-WORKSPACE.md) ·
[Native frame](../../disVM/runtime/editor_raster.h) ·
[Embedding tutorial](../examples/embedding/parallel_render/README.md)

The VM owner maintains the MovieClip hierarchy and executes AS/Dis operations.
It prepares immutable native painting commands; raster workers never traverse
managed objects or access fonts, channels, documents, GC, SDL or a VM callback.
A SWF timeline tick is one compatibility event source. It is not the execution
clock for native IDE input, typed-channel work or render submission. The host
may submit snapshots more or less often than timeline frames, or after a frame.
This does not promise that every player frontend already schedules all extensions
independently: this checkpoint implements that separation in the Dis desktop.

## Desktop preparation and execution

[`fvm_movieclip_capture_paths`](../include/flashvm/movieclip.h#L198) follows the ordinary protected MovieClip traversal,
including world transforms, ancestor color, visibility and signed child depth.
Its [`FVM_MoviePathSink`](../include/flashvm/movieclip.h#L186) receives borrowed solid contours, copied color, winding
rule and round stroke width. Copy each contour during the callback. Capture runs
on the VM owner; it neither advances the timeline nor rasterizes pixels.

The native desktop adapter adds copied text/gutter contours, selection/caret
commands, explicit clipping and opacity-group boundaries. Text shaping and AS
execution remain owner operations. Commands retain painter order derived from
the hierarchy; no managed display-list or font handle survives in a raster job.
An unsupported bitmap/gradient, non-round stroke or low-quality capture returns
UNSUPPORTED. Discard partial captured work on any error. Existing synchronous
MovieClip render APIs retain their behavior; this is not a complete arbitrary-SWF
snapshot serializer.

A persistent queue coordinator executes each accepted desktop snapshot off the
owner thread. It calls [`fvm_render_jobs_run`](../include/flashvm/render_jobs.h#L25) with **up to four tile workers** and
joins them on that background coordinator. Batch workers are created per batch;
this is not a persistent four-thread tile pool. Jobs contain only dirty tiles,
so clean canvas areas do not monopolize a worker partition. One dirty tile uses
the coordinator itself; an unchanged scene starts no tile workers.

## Damage and composition

Desktop pixels are divided into 64×64 tiles. Exact contour/scalar comparisons
against the **last displayed** snapshot identify old and new damaged bounds.
Common text-contour prefixes/suffixes keep an appended glyph from invalidating
all existing lines. Moving/removing paint invalidates its old location as well.
Size changes require a complete new image. There is no collision-prone content
hash deciding whether a region can be left unchanged.

Each dirty tile starts with the background and replays every intersecting layer
in original painter order, including unchanged foreground windows. Each worker
owns its pixel buffer, temporary translated paths and result slot. Clipping,
alpha and 90% window opacity groups are applied inside the tile. Clean tiles
retain copied pixels from the displayed baseline. Workers never mutate that
baseline. Superseded pending/executing snapshots cannot become an input baseline.

Only dirty tile rectangles are uploaded to the SDL texture. SDL composition,
window calls and optional PPM capture stay on the window owner. An expose event
re-presents cached pixels without asking Dis to rebuild the desktop. Full-image
copies still occur when establishing/transferring a baseline; this checkpoint
reduces rasterization and texture upload, not all memory traffic or owner layout.

## Input and cadence

Native text and key events use the last complete acknowledged Dis state and do
not wait for raster completion. Changes remain ordered through typed channels;
there is no speculative native document editing and no concurrent VM callback.
The SDL FIFO head is inspected without skipping pointer/focus/lifecycle events.
Pointer hit testing waits until displayed pixels match the committed geometry.

Consecutive queued keyboard events can replace unpainted desktop snapshots.
Painting is requested when that keyboard burst ends or its elapsed batching
interval reaches 16 ms at an owner poll. This interval is independent of SWF FPS
and is not a latency guarantee: scheduler, shaping, raster and display costs
still apply. Explicit replay/capture retains its deterministic presentation
barrier so existing view-by-view tests remain observable.

## Ownership and acceptance

[`fvm_render_queue_create`](../include/flashvm/render_queue.h#L32) starts 1–8 persistent native coordinators. The IDE uses
one coordinator. [`fvm_render_queue_submit`](../include/flashvm/render_queue.h#L40) transfers a complete job on success and
replaces older pending/ready work without waiting. [`fvm_render_queue_take`](../include/flashvm/render_queue.h#L47) with
wait=0 collects the newest completed result; wait=1 is for explicit capture/tests.
[`fvm_render_queue_destroy`](../include/flashvm/render_queue.h#L54) joins executing work before dependencies are freed.
The queue owns at most workers+2 accepted jobs. Obsolete executing jobs finish
normally and release their native storage; there is no forced cancellation.

[Capture regression](../../disVM/runtime/test_movieclip_capture.c) compares real
MovieClip pixels with captured solid/stroke commands after VM destruction.
[Dirty regression](../../disVM/runtime/test_editor_incremental.c) compares every
byte with independent full-surface drawing across tile edges, overlap, alpha,
movement, removal, appended contours and unchanged scenes.
[Native SDL regression](../../disVM/runtime/test_desktop_touch.c) verifies keyboard
admission during pending raster and FIFO/pointer/replay/capture barriers. Desktop
interpreter/JIT scenarios check real gestures, opacity and file ownership.
Dummy SDL/replay tests do not measure physical-device typing latency or certify
Windows/macOS/Android performance.
[Frame-free Async.post/cancel and SDL text bursts / Внекадровые callbacks](ASYNC.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_capture_paths` | function | [include/flashvm/movieclip.h:198](../include/flashvm/movieclip.h#L198) |
| `FVM_MoviePathSink` | callback typedef | [include/flashvm/movieclip.h:186](../include/flashvm/movieclip.h#L186) |
| `fvm_render_jobs_run` | function | [include/flashvm/render_jobs.h:25](../include/flashvm/render_jobs.h#L25) |
| `fvm_render_queue_create` | function | [include/flashvm/render_queue.h:32](../include/flashvm/render_queue.h#L32) |
| `fvm_render_queue_destroy` | function | [include/flashvm/render_queue.h:54](../include/flashvm/render_queue.h#L54) |
| `fvm_render_queue_submit` | function | [include/flashvm/render_queue.h:40](../include/flashvm/render_queue.h#L40) |
| `fvm_render_queue_take` | function | [include/flashvm/render_queue.h:47](../include/flashvm/render_queue.h#L47) |
<!-- source-links:end -->
