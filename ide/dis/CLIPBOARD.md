# Channel-owned Unicode copy, cut and atomic paste

[Русский](CLIPBOARD.ru.md) · [Reusable library](bridge/ideclipboard.m) ·
[Controller](examples/graphical-editor/main.b) ·
[Commented headless tutorial](examples/graphical-editor/clipboard-demo.b) ·
[Real GUI regression](../../disVM/runtime/test_graphical_clipboard.py).

Ctrl+C copies the selected source range. Ctrl+X deletes that range only after
the native owner acknowledges a successful clipboard write at the captured
revision/view identity. A failed/oversize copy leaves source, selection, history
and saved state intact. Copy/Cut with no selection leave the clipboard unchanged.
Ctrl+V reads one immutable native UTF-8 snapshot and publishes one document
Replace only after every fragment is admitted; Undo removes the entire paste.
Empty clipboard is a no-op, including over a nonempty selection.

The clipboard limit is 65536 UTF-8 bytes, independent of the document's scalar
capacity. NUL, invalid Unicode, oversize values, changed extent, duplicate/gapped
byte offsets and stale identity/revision reject the transfer before editing.
CRLF and non-BMP scalars are preserved. One Dis controller owns the captured
range, transfer identity and private staging array; reset/error/completion
retires that root. An unrelated input retires a pending transaction. Native
code never performs a source deletion, insertion or Undo operation.

Protocol extensions in the existing fvd1 envelope:

| Code | Direction | Copied fields |
| --- | --- | --- |
| 65554 ClipboardWrite | Dis View to native | flags=revision, first/second=ordered source scalar range, text=original ticket |
| 65556 ClipboardRead | Dis View to native | flags=revision, zero range, text=original ticket |
| 65557 ClipboardData | native Request to Dis | flags=captured revision, first=byte offset, second=total bytes, text=ticket+UTF-8 body |
| 65558 ClipboardAck | native Request to Dis | flags=captured revision, first=Ok/TooLarge/Invalid, zero second, text=ticket |
| 65559 ClipboardContinue | Dis View to native | captured revision, next byte offset/total, original ticket |

The ticket is exactly eight canonical hexadecimal ASCII digits. Body capacity
is 1016 bytes and splits stay at Unicode boundaries. Continue grants one next
fragment without retransmitting/rasterizing an unchanged document snapshot.
Native acknowledges that credit against its owned immutable transfer; no
cross-heap pointer, OS handle or native structure enters Dis. The native owner
frees read storage after copying the final frame. SDL clipboard access may block
the native UI owner; Dis waits for copied outcomes through its existing channels.

The native adapter uses SDL_GetClipboardText/SDL_SetClipboardText in interactive
mode. Ordinary --events/--window-events replay uses isolated process-local
clipboard values. Explicit --clipboard-events frames also exercise the actual
SDL clipboard API; automated tests select SDL_VIDEODRIVER=dummy. This distinction
keeps normal replay deterministic while testing the actual native adapter.
Physical desktop clipboard services and other-platform runtime ports remain
separate acceptance work.

Document clipboard shortcuts are currently inactive inside Find/Open/SaveAs/
GoTo fields; those fields retain their existing single-line input policy.
This is a matched component 0.6.0 host/controller/library extension. Rebuild and
ship all three together. Native file saving remains a separate explicit action.

The tests execute Unicode/CRLF copy, acknowledged Cut, exact 64-KiB fragmented
Paste with one Undo/Redo, oversize native Cut rejection and retired results.
Private staging tests independently cover stale/malformed/NUL/offset/size cases.
The native write result stays visible through key/modifier release; focus and
modifier observations do not erase the last operation diagnostic at the same
source revision. A new meaningful operation supplies its own status.

## Recompile the reusable value tutorial from an installed editor package

```sh
./limbo-native -g -I include/limbo -o clipboard-demo.dis examples/graphical-editor/clipboard-demo.b
./disvm --stats --jit clipboard-demo.dis
./disvm --stats --no-jit clipboard-demo.dis
```

The tutorial stages six UTF-8 bytes for `Я😀`, then three for LF+`AB`, and returns
one nine-byte/five-scalar value only on completion. It imports operation constants
from idehost.m without loading the native GUI host or touching an OS clipboard.
Our primary compiler/parser work remains the Coco/R Limbo/FIR pipeline; the
source-built legacy compiler here is explicit temporary build tooling.
