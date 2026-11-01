# Dense contour picking and selection presentation

[Русский](VECTOR-PICKING.ru.md) · [Object/node tools](VECTOR-EDITING.md)

The old picker tried figures in reverse paint order, then flattened every
quadratic into32 segments for stroke distance and even/odd fill-ray testing.
This ran in the Limbo owner, including curves nowhere near the pointer. Selecting
an imported dense contour could therefore consume substantial CPU before the
selection frame appeared.

The corrected path uses retained brush coverage for an interior hit in O(1),
provided its immutable mask has the exact admitted grid size. A missing/invalid
mask falls back to geometry. Conservative bounds include all quadratic controls
and are cached by immutable shape identity. Disjoint figures and curve control
hulls are rejected before subdivision. Remaining short curves use8 segments;
longer ones retain32. Stroke tolerance and even/odd holes remain explicit. Empty
release/return-to-origin restores the original array without walking nodes.

Selection presentation is now a separate owner cache in
`dis/bridge/idevectorpresentation.b`. Unchanged snapshots reuse the source and
overlay strings by reference through the existing bulk sender. Source-path bytes
are extracted once per geometry identity; only dirty selection overlays rebuild.
Pointer/mask/tool/selected-shape changes invalidate presentation; immutable array
identity also catches Undo and worker geometry publication. Idle Cancel does not
invalidate it. No mutable cache or Dis pointer reaches a raster worker.

The crash was a private protocol integration defect: node-control guides are open
Move/Segment subpaths followed by square-handle subpaths. The native reader had
required Close before the next Move. The last presentation path of editing views
now admits that valid open-subpath transition. Ordinary source-path restrictions,
coordinate/command/count limits and post-Close admission remain checked. Handle
coordinates are clamped on all four sides, including a rotate handle above the
page edge. SVG and Undo still contain source figures only.

Reproduction uses256 quadratic segments through actual Open, Objects, Nodes,
contour click and Save. The old installed v0.1724 fails with wire65577; the fixed
reader runs the same scenario in both modes. The test also draws a filled oval
and connected same-colour Brush ink, selects the union and switches to Nodes.
Saved output remains one object and excludes transient handles.

The optional `dis/examples/vector/pick-benchmark.b` times20 interior model clicks
on256 quadratics with no brush mask. With the same native x64 JIT executable,
the old module measured6390ms and the new module112ms. Initialization and GUI/
transport/raster time are outside this interval. It is one synthetic measurement,
not a universal speed factor or physical input-latency benchmark. Tests separately
check repeated Snapshot cache reuse and compare actual UI/persisted geometry.

Rebuild the complete matching `disvm-editor` package: the private Limbo document
layout changed. No shared SWF/Dis64 registry or file-native admission changes.

From the installed runtime package directory, using its matching native-profile
compiler and module interfaces:

```sh
./limbo-native -I include/limbo -o pick-benchmark.dis examples/vector/pick-benchmark.b
./disvm --jit --max-instructions 50000000 pick-benchmark.dis
```

The installed example measured105ms on this host. Use your own result to compare
like-for-like builds; do not compare different sources, modes or hardware as one
performance factor.
