# Background native rendering

[Русский](RENDER_ASYNC.ru.md) · [Public API](../include/flashvm/render_queue.h) ·
[Implementation](../src/render/render_queue.c) ·
[Runnable C89 tutorial](../examples/embedding/parallel_render/README.md) ·
[IDE frame implementation](../../disVM/runtime/editor_raster.c)

[`fvm_render_queue_create`](../include/flashvm/render_queue.h#L32) starts 1–8 persistent platform workers. The graphical
Dis/Flash editor uses **one background raster worker**. Text layout, glyph outline
extraction, AS execution, hit geometry and GC stay with the VM owner. The worker
receives detached native paths/colors and its own RGBA buffer. SDL texture upload,
window operations and optional PPM capture stay on the window owner.

This moves actual editor path rasterization off the execution thread. It does
not move layout or all display-list traversal off that thread, partition a single
frame among multiple workers, or provide a complete asynchronous SWF player.
The direct Dis → typed channels → Flash content route is unchanged.

## Queue contracts

| Operation | Ownership and waiting |
| --- | --- |
| [`fvm_render_queue_create`](../include/flashvm/render_queue.h#L32) | Validate callbacks/worker count; return an owned queue only on success. Partial thread creation is rolled back by joining started workers. |
| [`fvm_render_queue_submit`](../include/flashvm/render_queue.h#L40) | Transfer a distinct nonnull job on success. No wait for rasterization; discard the previous pending/ready job. Allocation failure leaves the job with its caller. |
| [`fvm_render_queue_take`](../include/flashvm/render_queue.h#L47) | With wait=0, return NOT_FOUND until the latest job completes; outputs stay unchanged. On OK transfer the job and its separate execution status back to the owner. wait=1 is an explicit capture/test barrier. |
| [`fvm_render_queue_destroy`](../include/flashvm/render_queue.h#L54) | Stop pending work and join executing callbacks before freeing synchronization. May block; callbacks must terminate. Null is accepted. |
| [`FVM_RenderExecute`](../include/flashvm/render_queue.h#L19) | Borrow one exclusive native job; return its real status. Different jobs may execute concurrently. |
| [`FVM_RenderRelease`](../include/flashvm/render_queue.h#L26) | Release only native owned storage; may run on an owner or worker. No VM/GC/SDL access, reentry or retained pointers. |

All public calls require one host owner, with no concurrent destroy or callback
reentry. Internally a port mutex/condition publishes completion. Idle workers sleep
on the condition, not a spin loop. No new OS API leaks into the core: Linux,
Android and macOS use their existing POSIX adapters, Windows its Win32 adapter.
Only the Linux execution is checked in this checkpoint.

The queue retains at most workers+2 jobs: executing work, one replaceable pending
job and one ready result. Already executing obsolete jobs are allowed to finish;
they are not forcibly interrupted. Only the newest accepted job can be taken.
Disposal runs outside the queue lock, but can consume owner time when submit
replaces pending/ready native storage. The API is not a hard real-time guarantee.
The host must bound each payload and avoid creating an unbounded set of queues.

## Editor integration

The owner completes all Dis envelopes and AS view updates before preparing a
frame. Native paths returned by TextField/font helpers are independent of the
VM/font after extraction. Clipped contours and selection/gutter/panel rectangles
are transferred in painter order, not pointers to live objects. Frames are bounded
by 2048×2048 pixels, 8192 painted paths and 262144 contour commands; budget or
allocation failure is reported rather than queuing a partial frame.

The owner collects with wait=0 while polling the normal editor transport. A
completed frame of an obsolete canvas size is discarded. While pixels are pending,
native input stays in the OS queue so a click cannot use new hit geometry against
old displayed pixels; Dis scheduling/channel polling continue. Replay input advances only after owner presentation through nonblocking polling.
`--capture` explicitly waits for raster completion; this output barrier does not
make ordinary interactive or uncaptured replay submissions wait. Shutdown joins workers before fonts or either VM
are destroyed. Native raster jobs contain no managed roots or shared VM heap.

## Verification and limits

- [Queue regression](../tests/async_render_jobs.c): two parked workers prove that
  owner submission/nonblocking take proceed without raster progress; pending
  replacement, stale-result disposal, failure delivery and exactly-once release.
- [Pixel regression](../../disVM/runtime/test_editor_raster.c): real AGG pixels
  match synchronous execution byte for byte; opaque/blended interior pixels also
  have independently checked colors and painter ordering.
- Existing graphical editor replays cover Unicode/caret/clipboard, resize, gutter,
  browsing and interpreter/JIT paths. SDL dummy checks do not prove physical
  desktop/touch performance or Android/macOS/Windows acceptance.
- [Joined-runner failure test](../tests/render_jobs_failure.c) injects failure at
  each launch. Every remaining initialized partition must run once before error.

The existing [`fvm_render_jobs_run`](../include/flashvm/render_jobs.h#L25) remains a joined batch API. It may be used by a
host that explicitly needs a barrier; it does not become asynchronous by creating
threads. See [both execution models](RENDER_THREADS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_render_jobs_run` | function | [include/flashvm/render_jobs.h:25](../include/flashvm/render_jobs.h#L25) |
| `fvm_render_queue_create` | function | [include/flashvm/render_queue.h:32](../include/flashvm/render_queue.h#L32) |
| `fvm_render_queue_destroy` | function | [include/flashvm/render_queue.h:54](../include/flashvm/render_queue.h#L54) |
| `fvm_render_queue_submit` | function | [include/flashvm/render_queue.h:40](../include/flashvm/render_queue.h#L40) |
| `fvm_render_queue_take` | function | [include/flashvm/render_queue.h:47](../include/flashvm/render_queue.h#L47) |
| `FVM_RenderExecute` | callback typedef | [include/flashvm/render_queue.h:19](../include/flashvm/render_queue.h#L19) |
| `FVM_RenderRelease` | callback typedef | [include/flashvm/render_queue.h:26](../include/flashvm/render_queue.h#L26) |
<!-- source-links:end -->
