# Moving and retiring desktop windows — v0.1674

[Desktop guide](DESKTOP.md) · [Русский](DESKTOP-LIFECYCLE.ru.md) ·
[Portable example](dis/examples/desktop-lifecycle/README.md)

Drag a window's first 24 pixels (its title) to move that window. Drag empty table
space to scroll the table. A body gesture does neither: mouse caret selection is
still a separate integration task. The focused window paints last and wins
hit testing in the same overlap. Other windows keep stable slot order.

Use the title's `x`, Ctrl+W, or the right Close tool to close an editor. A clean
editor retires immediately through Ctrl+W/title close. A dirty editor opens a
modal decision: **Discard this editor** retires that owner, **Cancel** or Escape
keeps it. To save instead, cancel, use Ctrl+S, then close again. Help and the build
console are permanent system panes; close never substitutes global Quit.

![Real dirty-close decision](assets/desktop-v0.1674.png)

## Ownership and ordering

1. Flash input supplies bounded pixels/phases and the completed view ticket.
2. `Idedesktop` admits a title drag or close proposal. It owns geometry, not text.
   A close proposal alone never removes a pane.
3. The application asks that pane for an actual revision/saved snapshot. While a
   dirty decision is modal, edits, focus changes, other tools and window creation
   cannot change its target. Explicit Cancel/Escape or discard resolves it.
4. On admitted close, the application consumes the pane's terminal `Shutdown`
   acknowledgement, drops its channel, then asks layout to `Remove` the slot.
5. `Remove` omits that window, its title close button and its live miniature from
   one complete scene. It cancels an active gesture and chooses surviving focus.
6. The FlashVM owner clears absent pooled MovieClips, caret paths and TextFields;
   actual empty-text readback is checked. Empty clips remain in a bounded pool for
   reuse. This does not claim per-close destruction of the FlashVM context.
7. New creates a new document process in a free slot. It does not reuse the
   retired document channel, undo history, path, dirty state or layout offsets.

`Remove` refuses system slots 2/3, absent slots and out-of-range identities.
The registry still has eight total slots, including Help and console. Creating
more editors reports the limit in the console. Native IDs never become channels
or source capabilities. All operations use matching typed `.m`/`.dis` modules;
rebuild and install them together after an interface update.

## Gestures and bounds

Coordinates are table-local; the table's scroll transform is applied separately.
Offsets survive source/view updates and remain in memory for this application
session. Positions are clamped when presented on a resized canvas. Restart and a
new owner start from the grid; there is no persisted session file yet.

Stale tickets, focus loss, Cancel, wheel scrolling, resize, selection and removal
cancel active gestures. A title close press must release inside the same control;
a release outside or a moved press does not close. Expanded scroll bodies block
hits on covered windows. Narrow layouts use the rendered button bounds too.

Native MovieClip painting remains on the FlashVM owner; only completed pixels go
to detached presentation. Independent Limbo processes are cooperatively scheduled,
not parallel native VM threads. No new ABI, SWF/Dis container or parser is added.

## Verification

Portable tests exercise drag/update preservation, cancellation, close proposals,
remove/recreate, system protection and narrow cancel bounds. Actual FlashVM replay
checks transforms, overlap order, dirty cancel/Escape/discard, blocked edits,
TextField retirement, live thumbnails and ten clean close/new cycles. Both native
AMD64 interpreter and JIT execute the same clients. SDK checks also run the
portable tutorial and graphical shell outside repository paths. Physical touch,
other CPU/OS execution and attached Run/Debug remain separate acceptance work.
