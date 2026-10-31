# Retained IDE object bitmaps and dirty composition

[Русский](RETAINED-SCENE.ru.md) · [Rendering ownership](../../ide/RENDER_OWNERS.md) ·
[Desktop gestures](../../ide/DESKTOP-WORKSPACE.md)

The desktop now uses associated **local RGBA bitmaps per admitted window/widget**.
Content, placement/opacity and descendant composition have separate dirty flags.
The bitmap contains the object's own ink; descendants remain separate painter-
ordered layers, preventing the old MDI double-capture/ghost problem. The final
canvas is the composition root. This first cache granularity is the desktop
protocol's visual objects, not a separate allocation for every glyph or vector verb.

## Work and ownership

The FlashVM owner creates a native snapshot of local commands, identity, parent,
composed affine transform and opacity. No managed object/font/VM pointer reaches
workers. Unchanged local content shares an immutable image with the last actually
**displayed** frame. Superseded queued frames cannot become the damage baseline.
A short mutex protects native image reference counts only; pixel work holds no lock.

Up to four native workers rasterize changed local images. Another packed worker
pass composites only dirty64×64 root tiles from the ordered bitmap layers. Moving,
scaling, rotation and opacity do not repaint unchanged image content. Integer
translation uses direct pixel sampling and opaque copies; fractional transforms
use premultiplied bilinear filtering to avoid transparent-edge color fringes.
Opacity uses a1000-unit scale, so dragged paper remains exactly90 percent.

Changed child content propagates composition bits through all recorded parents.
Parents' unchanged own ink is retained; a child change does not invalidate its
parent's body bitmap. Old/new transformed bounds, deletion and painter-order
changes damage the root. All overlapping cached layers are composited in those
regions, including unchanged foregrounds. This is necessary for transparency and
revealed content. Successful root composition clears the candidate's propagated
node flags. Texture-upload damage remains available until owner presentation.
Failures never mutate an already displayed snapshot or clear its history.

Inside an edited bitmap, unchanged contours are retained. Changed complete glyph/
path contours determine a local damage rectangle. The worker copies the immutable
base image, clears that rectangle and replays only clipped overlapping local ink.
This keeps text edits from repainting an entire window. A full bitmap allocation/
copy can still occur for a changed object; this is not a tiled COW pixel allocator.

For complete, canvas-contained ordinary source/help/console panes, an exact copied
content key also skips owner-side MovieClip actions and scene reconstruction.
The associated image retains its immutable local commands too: the owner
reuses references with the new transform instead of cloning/comparing glyph
outlines again. Changed images alone copy their canonical command chain. Mirror/vector/clipped
panes and active visual animations still prepare owner commands; their native
bitmap cache is independently compared/reused where local content remains equal.
Canonical local geometry uses a1/4096-pixel grid to remove inverse-transform
roundoff; cache matching is exact on that representation, not a lossy hash.

## Input and presentation

A continuing **captured mouse header drag** may deliver latest absolute positions
while raster work is pending. It does not need a new hit test. FIFO head inspection
never skips fresh clicks, releases, focus or lifecycle boundaries. New hits still
wait for matching pixels, as do deterministic replay/capture. Native keyboard
input retains its existing frame-free path. Other touch/selection gestures keep
their existing geometry barrier; this change is not a universal input bypass.

SDL requests accelerated presentation and falls back to software when unavailable.
The root texture is still uploaded by dirty tiles. Associated object composition
currently runs on the CPU; this is not a GPU scene-graph backend. Full frame-buffer allocation remains for immutable queue isolation. In v0.1702,
only clean spans are copied into a successor and dirty spans into the owner
buffer. Exact native baseline identities force a full copy/upload when a different
frame was shown meanwhile, or the owner buffer was recreated. Desktop GC now follows the normal safe-point allocation
threshold instead of forcing a complete sweep for each drag update.

## Limits, fallback and observation

Stable IDs1..63, acyclic native parents, bounded transforms and local image extents
are checked before submission. Associated bitmap storage is limited to64MiB per
snapshot. On this budget limit the desktop retries the ordinary vector/tile path,
retaining opacity and input behavior. Malformed scenes do not receive a successful
fallback. Mode changes require a complete baseline rather than diffing canonical
local commands against world-space vectors.

A retained frame holds references to immutable associated images; changed image
candidates never overwrite old pixels. Queue replacement, shutdown and base-frame
destruction release their references safely. The diagnostic physical image count
must return to zero after every frame and queue owner is destroyed.

Set `DVM_EDITOR_RENDER_STATS=1` before launching the installed IDE to print counts
without source text:

```sh
DVM_EDITOR_RENDER_STATS=1 ./run-desktop-ide.sh
```

`SCENECACHE ticket built reused dirty local_pixels` describes actual associated
bitmap builds/reuses, initially dirty nodes and local repainted pixels.
`ROOTDAMAGE ticket tiles pixels` describes root composition/upload tile coverage.
These are work counts, not an FPS/latency measurement. Deterministic replay emits
them automatically. `NODECACHE` in replay identifies an owner reconstruction skip.
For development comparison only, `DVM_EDITOR_RETAINED=0` selects the ordinary
vector path; it preserves the same controller/document policy.

## Verification and remaining scope

Native tests compare fresh and incremental RGBA for movement, opacity, zoom,
rotation, removal, order and one-object content invalidation. They assert zero
content rasterizations for transform-only updates, contour-local text patching,
ancestor propagation/clear, immutable base lifetime, queue replacement/shutdown,
cycle/budget refusal and final physical image count0. Real IDE replay verifies
owner skips, full bitmap reuse while dragging, selective text invalidation and
bounded root damage. Native SDL FIFO tests cover continued captured motion and
fresh-pointer/replay barriers. SDL dummy-driver tests exercise software fallback;
physical GPU/desktop latency requires a device run.

Per-subtree precomposed group textures, separate glyph/primitive object nodes,
external opacity maps, pixel-tile COW, resolution-adaptive bitmap rebuilding after
large zoom and GPU object composition remain future work. Associated bitmaps are
at local resolution; bilinear enlargement does not add vector detail. Existing
source owners, MDI/SDI membership and shared Dis/SWF formats remain unchanged.

`FRAMECOPY ticket baseline_bytes presented_bytes` reports actual pixel transfer
bytes (no padding). For a compatible valid baseline the two disjoint selections
sum to one canvas, rather than two complete canvas copies. Fresh/stale buffers
require full presentation; the counter does not claim zero-copy or measure FPS.

[Transfer verification / Проверки переноса](../../docs/releases/v0.1702.md).
