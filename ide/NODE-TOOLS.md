# Contour node bar and graphic clipboard

IDE0.7.15 / workspace v0.1731. [Русский](NODE-TOOLS.ru.md).
Choose **Nodes** on the left. The horizontal bar above the page has these icons,
from left to right. The diagrams are original small vector glyphs using the
familiar node-operation vocabulary; they are not copied CorelDRAW assets.

| Button | Operation |
| --- | --- |
| Add | Split each selected anchor's following explicit span at t=1/2. Cubics use de Casteljau; controls remain editable. |
| Delete | Remove selected anchor commands. |
| Break | Duplicate selected interior anchors as separate subpath endpoints. A closed path is rotated to the cut and opened without refitting its segments. |
| Join | Weld adjacent anchors/endpoints at their midpoint, or close a single open contour by welding its start/end. Other nonadjacent interiors are refused. |
| Line | Convert the span between two adjacent selected anchors to a straight segment. |
| Bezier | Convert that span to a cubic. A line becomes collinear cubic controls; Q-to-C conversion retains its curve up to integer rounding. |
| Cusp | Opposite handles are independent. |
| Smooth | Handles remain collinear and opposite; rotating one preserves the other's own length. |
| Symmetric | Handles remain collinear/opposite with equal length; rotation and length changes mirror together. |

Click/Ctrl-click or drag a marquee to select anchors/controls. Operations act on
anchor selection; an impossible selection is refused without a history change.
Each accepted operation is one Undo transaction. Source arrays remain immutable.
Smooth/symmetric relationships are stored with shapes, survive Undo and SVG
round trips through optional `data-flashvm-joins` digits0/1/2. Ordinary SVG readers
may ignore that editor metadata; cubic geometry itself remains standard SVG.
Controls and reflected points must remain inside the1000×600 logical page.

**Delete / Backspace** removes selected anchor groups in Nodes. If no node/handle
is selected, it removes the whole selected figure/compound contour. A control-only
selection is not mistaken for a whole-figure deletion. Deleting every usable
anchor removes its figure. The Delete bar command is the explicit node operation.

**Ctrl+C / Ctrl+X / Ctrl+V**, and the existing Edit palette, use the desktop's
session clipboard for graphics too. Copy exports the selected complete figure as
self-contained SVG. Cut removes it only after successful clipboard acknowledgement.
Paste validates the SVG before editing: a selected figure is replaced at its
z-order; otherwise the input is appended and selected. A refusal leaves geometry
unchanged. The existing clipboard limit is64KiB UTF-8; the selection/revision
must match a fresh reply. This is the IDE session clipboard, not OS clipboard
integration or multiple independently selected objects.

## What no longer waits for a frame/reply

A selected Nodes contour publishes an idle native modifier recipe. A plain hit
on a visible anchor/handle can begin local motion **before** Dis acknowledges
Begin. The owner still receives Begin and one final release for history. Ctrl or
unprepared hits use normal admission. Matching canonical acknowledgement keeps
the current pointer instead of rewinding. A release followed by idle state retires
the baseline even for a tap without movement. Partial snapshots remain fenced.

Admitted object translation, corner scale, rotation and node-marquee frames also
update from immutable native baselines, without intermediate Dis Motion messages.
Only final geometry/selection is committed by Limbo at release. Native rendering
requests use the existing frame-free owner callback and detached raster workers.
A near-capacity source without room for its overlay safely retains the reference
Dis route; this is not a claim that every IDE interaction is now local/asinc.

The node overlay uses a generic C binding renderer: source prefix, guide endpoints
and marker corners are reconstructed from copied owner values. Limbo does not
serialize every redundant decoration coordinate. An unchanged selection reuses
its overlay and only rewrites the small origin/revision header. Public SWF/Dis64
formats are unchanged; private desktop flags0x80000/0x100000/0x200000 identify idle,
compact-decoration and extended modifier plans. Rebuild/install matching modules
and native executable together.

## Reproduce performance and tests

`dis/examples/vector/pick-benchmark.b` separates actual model selection from
presentation construction with Sys.millisec. On this x64 host, JIT enabled, twenty
Begins on256 quadratics took123ms selection /26002ms presentation before these
optimizations. After cache plus compact bindings:129ms /374ms; ASCII payload
172632→39232 bytes. These are local aggregate model timings, not measured phone
latency, network timing or a frame-rate guarantee.

The native SDL fixture proves movement before the Begin reply and no intermediate
Dis proposal for600 node movements and100 movements each for scale/marquee.
Portable model/pane tests cover actual operations, tangent constraints, keyboard
and clipboard refusals in interpreter/JIT. Independent native modifier tests
cover geometry, clipping and rollback. Existing SVG/pixel desktop replays remain
the full-channel reference route.

**Thread status:** extended up/REG TLS remains present. The current graphical host
still alternates its Dis scheduler and FlashVM event pump on one OS thread.
Raster workers are real OS threads. Full multi-owner Dis scheduling/GC/channels
and AS asinc capture lowering remain separate unfinished runtime work.

[Implementation](dis/bridge/idevectornodes.b) · [Curve helpers](dis/bridge/idevectornodecurve.b) ·
[Cutting](dis/bridge/idevectornodebreak.b) · [Clipboard](dis/bridge/idevectorclipboard.b) ·
[Cache](dis/bridge/idevectoroverlaycache.b) · [Generic transforms](../disVM/runtime/vector_transform.c).
