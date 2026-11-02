# Native UI and Dis execution owners

Runtime0.40.0 / IDE0.7.16 / workspace v0.1732.
[Русский](THREADING.ru.md) · [Runnable managed example](https://github.com/bhgv/flashVM/blob/master/disVM/examples/threaded-owner/README.md).

`disvm-editor` now runs the complete Dis interpreter/JIT, loader, document
services, channels and collector on a dedicated OS thread. SDL, FlashVM objects,
input, local graphic gestures and presentation remain on the launching main
thread. Existing detached raster workers continue independently. This is the
default `--threaded-ui` mode; `--sequential-ui` selects the retained reference
execution route. Duplicate execution-mode flags are refused before launch.

## Ownership and queue boundary

| State | Exclusive owner |
| --- | --- |
| extended up, active REG, error frames, process FIFO, module/heap/GC state | Dis OS worker |
| SDL, FlashVM context, MovieClips, current scene, native live/frozen ink registry | Main/UI |
| detached raster snapshots | Existing raster workers |
| transport records | Copied bounded channel values |

`editor_thread.c` launches/join-controls the Dis entry and owns one synchronized
RPC slot. `editor_host.c` separates the Dis service pump from the UI pump;
`editor_bulk.c` forwards copied geometry to the same bounded UI queue. Main never
reads a Dis channel, Prog, stack, module or heap object. Dis never calls the UI
context, scene or ink registry directly in threaded mode.

The shared transport retains32 records per direction and the local input FIFO.
Bulk send can block its Dis producer under backpressure; main/UI keeps running.
The existing logical End/deferred-geometry fence protects partial scenes. A
queue receipt means copied transport acceptance, not completion of a document
operation. Native views keep their existing local gesture/frame-free rendering
contracts; this change separates OS scheduling from document reply latency.

Startup rows/preferences/path and ink read/ack/cancel use owner RPC. The caller
copies generated native frames before waiting. Input bytes are copied into native
storage, bounded at2MiB; UI replies with copied scalars or malloc-owned text.
Only frozen ink is returned. No native/managed address is a wire payload. The
Dis caller may wait; UI never waits on this RPC. Returned text is freed even if
installing the Dis string raises an allocation error. The original mapped frame
continues rooting the native call's arguments/result during that wait.

## Lifecycle and errors

Main parses options before thread creation. The worker starts its own extended
up/REG TLS and initializes the real runtime there. UI construction runs on main
through startup RPC. Startup/transport failures become a controlled runtime
failure and trigger owner shutdown. Closing queues rechecks synchronized stopping
state to distinguish normal EOF from a transport failure.

The Dis owner detaches managed channel roots and destroys its pumps. Its entry
returns normally, then it unbinds/destroys native TLS before publishing completion.
Main joins before freeing transport, RPC storage, UI state or borrowed arguments.
UI failures close queues so backpressured Dis sends wake. Cancellation remains
cooperative; a blocking uncooperative native callback can still delay shutdown.
Hard allocator/process exits retain their existing process-fatal semantics.

For diagnostics set `DVM_EDITOR_THREAD_STATS=1`. A normal run reports checked
worker binding/retirement, main's absent Dis TLS and main-loop poll count:

```
NATIVEOWNERS ui=main dis=worker ui_polls=... main_dis_tls=0 worker_bound=1 worker_released=1 result=0
```

Counts are observations, not a frame-rate/latency guarantee. The main thread can
serve input/native local gestures while Dis computes; ordinary commands which
need document authority still await its reply.

## Scope and validation

The extended-up/REG preparation is used directly through native_process and one
native_context TLS owner. Real managed applications, interpreter/JIT counters,
collector activity and normal returns are checked in8 separate processes; module
and UI initialization failures check retirement, and duplicate flags are rejected.
Actual desktop SVG/pixel editing and clipboard/tool replays validate the threaded
UI route. A sequential replay remains a reference. Register/world/Fast-Cycles
coordination fixtures retain their focused, separate evidence.

There is one Dis heap mutator/scheduler owner in this stage. Limbo spawn tasks
remain cooperative on it. The world/safepoint and GC-worker infrastructure remains
available for parallel shared-heap Dis scheduling, actual collector integration,
blocking Sys release/reentry, module/JIT publication and synchronized heap metadata.
These are the next runtime stages; this commit does not enable unsynchronized
multiple Dis mutators or executable AS asinc capture lowering.

Only native Linux x64 has been executed in this session. Common thread-port use
does not constitute physical ARM64/RV64, Android or desktop-display acceptance.
