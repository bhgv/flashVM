# Joined and asynchronous native rendering

[Русский](README.ru.md) · [Every-line-commented C89 source](main.c) ·
[Queue guide](../../../doc/RENDER_ASYNC.md) · [Joined batches](../../../doc/RENDER_THREADS.md)

This executable demonstrates two different host contracts. First four native
workers fill 64 independent pixels and [`fvm_render_jobs_run`](../../../include/flashvm/render_jobs.h#L25) joins them. Then a
persistent background worker fills a separately allocated image: submit returns
without waiting, the owner continues, and an explicit final take waits only because
this console tutorial must check output before exiting. An interactive host uses
wait=0 and continues its VM/event loop on [FVM_NOT_FOUND](../../../include/flashvm/flashvm.h#L49).

This is a queue/ownership tutorial, not a complete MovieClip renderer. The current
IDE's actual native path rasterization uses the same queue; see the guide for its
frame bounds, hit-geometry barrier, replay/capture behavior and platform limits.
The companion [example.as](example.as) illustrates ordinary owner-thread script
updates; it does not start threads or replace the C executable.

## Build and run

Use a configured FlashVM build with RENDER_JOBS selected in DEFS.txt and examples
enabled. See [workspace preparation](../../../../README-FIRSTSTEPS.md) for SDK setup.

```sh
cmake --build /path/to/flashvm-build --target flashvm_examples_embedding_parallel_render_main
/path/to/flashvm-build/flashvm_examples_embedding_parallel_render_main
```

Expected output:

```text
Owner continues after submit; native raster may still be running.
PASS joined batch and asynchronous native snapshot
```

## API and cleanup

| Function | Contract demonstrated |
| --- | --- |
| [`fvm_render_jobs_run`](../../../include/flashvm/render_jobs.h#L25) | Borrow the stack image only until joined return; each callback owns disjoint pixel indices. |
| [`fvm_render_queue_create`](../../../include/flashvm/render_queue.h#L32) | Own one persistent queue/worker; callbacks stay valid until destroy returns. |
| [`fvm_render_queue_submit`](../../../include/flashvm/render_queue.h#L40) | Transfer a heap snapshot only on success; keep and free it yourself on failure. |
| [`fvm_render_queue_take`](../../../include/flashvm/render_queue.h#L47) | Return a completed allocation to the owner, separately reporting the worker status. A returned failure result still needs disposal. |
| [`fvm_render_queue_destroy`](../../../include/flashvm/render_queue.h#L54) | Join workers and dispose untaken jobs before dependencies are destroyed. |

Callbacks only access private native bytes. They may not enter either VM, access
SDL/font state, throw or reenter the queue. `release` is safe on worker and owner.
The sample nulls the transferred pointer, and its single cleanup block frees only
caller-owned pending/completed storage. Queued storage belongs to destroy or take,
never both. No physical window or device is required.

[Example catalog](../../README.md#example-embedding-parallel_render).
## Actual desktop integration

The application path is described in [event-driven rendering](../../../doc/RENDER_ASYNC.md).
The VM owner captures real MovieClip contours with
[`fvm_movieclip_capture_paths`](../../../include/flashvm/movieclip.h#L198); the sink copies each borrowed contour before returning.
The desktop adds immutable text, clipping and opacity commands. A persistent
coordinator invokes the parallel batch away from the VM owner. Only damaged
64×64 tiles are drawn and uploaded; unchanged overlapping layers are replayed
inside damaged tiles. Timeline ticks do not pace keyboard/channel callbacks.

Two runnable C regressions complement this embedding tutorial:
[VM-independent capture](../../../../disVM/runtime/test_movieclip_capture.c) destroys
the VM before drawing captured contours; [dirty tile comparison](../../../../disVM/runtime/test_editor_incremental.c)
checks every pixel against full drawing, including an appended local contour.
They are built as `disvm_movieclip_capture_test` and
`disvm_editor_incremental_test` by the graphical Dis runtime test configuration.
The capture extension currently supports solid fills and round strokes, not
arbitrary SWF bitmap/gradient capture. Partial snapshots must be discarded on failure.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_capture_paths` | function | [include/flashvm/movieclip.h:198](../../../include/flashvm/movieclip.h#L198) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../../../include/flashvm/flashvm.h#L49) |
| `fvm_render_jobs_run` | function | [include/flashvm/render_jobs.h:25](../../../include/flashvm/render_jobs.h#L25) |
| `fvm_render_queue_create` | function | [include/flashvm/render_queue.h:32](../../../include/flashvm/render_queue.h#L32) |
| `fvm_render_queue_destroy` | function | [include/flashvm/render_queue.h:54](../../../include/flashvm/render_queue.h#L54) |
| `fvm_render_queue_submit` | function | [include/flashvm/render_queue.h:40](../../../include/flashvm/render_queue.h#L40) |
| `fvm_render_queue_take` | function | [include/flashvm/render_queue.h:47](../../../include/flashvm/render_queue.h#L47) |
<!-- source-links:end -->
