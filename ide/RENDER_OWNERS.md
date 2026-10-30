# Dis channels, AS presentation and parallel native rendering

Current implementation: the desktop owner captures the MovieClip hierarchy into
immutable native paint commands. A persistent background coordinator distributes
only damaged 64×64 tiles among up to four native workers; only the latest result
is presented. Native keyboard input and render submission are independent of SWF
FPS. Layout/AS/GC and SDL remain on their owners. [Queue contract](../flashVM/doc/RENDER_ASYNC.md).

[Русский](RENDER_OWNERS.ru.md) · [Native C modules](../disVM/runtime/NATIVE_MODULES.md) ·
[AS2 → FIR → SWF](../compiler/as2/view/README.md) ·
[Native render-job API](../flashVM/include/flashvm/render_jobs.h)

The direct route **DisVM → typed channels → Flash content** remains available.
AS is not a mandatory proxy for every property or display command. Moving complex
presentation from C to AS changes who assembles widgets, not the channel contract.
The application/document/broker logic remains Dis/Limbo; AS may implement visual
composition and widget behavior in FlashVM. C supplies extensions such as bounded
colorizers and rendering primitives, plus OS ports.

| Work | Owner and permitted concurrency |
| --- | --- |
| Document, revision, request routing | Limbo service; typed channels remain authoritative. |
| Live AS objects, callbacks, display mutations | The owning FlashVM context, serialized. |
| Independent AS workers | Separate contexts/heaps and checked value messages; not shared mutable VM state. |
| Raster jobs for independent clips/regions | Immutable inputs, disjoint outputs; execution may be parallel. |
| Result application and frame publication | Owner checks revision/generation, ordering and cancellation. |

AS1/2's original single event/execution owner does not force pixel work into one
thread. The existing native `fvm_render_jobs_run` is a synchronous parallel-job
primitive: its callbacks must not execute live AVM1 or mutate shared display
objects, and it joins before returning. It is not itself a complete SWF renderer.
Thousands of clips/dirty regions should be partitioned into bounded batches or a
pool; they should not cause an unbounded thread per object.

For native colorization, copy a bounded source window and its language, revision,
Unicode base and request identity. C computes ranges without calling VM APIs.
Return copied validated ranges. The owner discards stale/cancelled results and
publishes only to the matching view. A work queue must define backpressure and
buffer lifetime. Full asynchronous colorizer delivery and migration of all editor
widgets are future integration, not established by the simple AS display profile.

The current graphical editor already communicates through Dis channels and runs
its authored AS view through FIR/SWO/SWL/SWF. Some legacy glyph/range painting
remains native. C modules do not acquire a GUI shortcut by being native. The two
VM heaps stay separate; a native pointer cannot become a channel capability.

The experimental reference renderer in
[the separate graphics repository](https://github.com/bhgv/inferno-os_gfx-engn/tree/master/libinterp/graph)
was reviewed without importing its Cairo/GUI engine into DisVM. Its repaired leaf
handoff, bounded batch scheduling and owner-only shared text helper illustrate
useful boundaries. It still requires an immutable, acyclic graph for a render
call, retains fixed scratch-canvas limitations and cooperative join polling, and
is not proof of fully concurrent graph mutation or complete platform acceptance.

Graphics-fork publication status / Статус графической ветки: fixes are ready as
local commit `51b2ebaf19e1bd0002974b5da8d55b0cab7ca6cd` (tree
`74df790b3e4e072e574ed6b3e4e9f6cb9da56f04`). GitHub integration returned
`403 Resource not accessible by integration`; direct Git has no credentials.
They are supplied as a tested backup patch, **not claimed published** to the fork.
Исправления приложены проверенным патчем; публикация в графический fork пока
недоступна, в отличие от разрешённой публикации flashVM.

[Retained IDE bitmap tree / Сохраняемые битмапы IDE](../disVM/runtime/RETAINED-SCENE.md).
