# Dis-owned Unicode input fields

[Русский](FIELDS.ru.md) · [Interface](bridge/idefield.m) ·
[Implementation](bridge/idefield.b) · [Controller](examples/graphical-editor/main.b) ·
[Runnable tutorial](examples/graphical-editor/field-demo.b) · [Clipboard](CLIPBOARD.md).

Find, Open, Save As, replacement and Go To use one reusable Idefield state.
The Limbo controller exclusively owns its text, caret, anchor and monotonic
generation. Field edits do not send document Replace commands, create Undo
entries, change saved revisions or change the current file path. Enter submits
the accepted value to the appropriate document/file operation; Escape retires
the field. A window close still protects unsaved source while a field is open.

Positions count Unicode scalars, not UTF-8 bytes. Reverse selections are valid.
Left/Right, Home/End, Shift selection, Ctrl+word movement, Ctrl+A, Backspace,
Delete and Ctrl+word deletion share the source editor's word-boundary policy.
Text replaces the selected range. There is no private field Undo stack yet.

Each complete field is limited to 256 scalars. NUL, surrogate/non-Unicode values,
tab and hard line separators (CR, LF, VT, FF, NEL, LS, PS) are rejected before
mutation. An oversized or invalid insertion preserves text, both endpoints and
generation. Generation exhaustion returns Invalid without wrapping; genuine
no-ops do not advance it. Starting a new modal session allocates fresh state.
An invalid initial path prefill leaves an empty field and a visible diagnostic;
it never truncates or silently changes the document's native path.

## Presentation and pointer boundary

The matched runtime component is 0.7.0. Ship its host, controller, Idefield,
Ideinput and interfaces together. This extends the existing channel vocabulary,
not the SWF or Dis file format.

| Code | Direction | Contract |
| --- | --- | --- |
| 65560 PanelSelection | Dis View → native | flags=document revision; first=field caret, second=anchor; empty text |
| 65561 ClipboardFieldWrite | Dis View → native | captured revision/ticket and ordered field scalar range; excludes the label |
| 65562 FieldPlace | native Request → Dis | revision/ticket; first=proposed scalar or -1 outside footer; second=begin/update/end (1/2/3) |

Panel text and selection precede the final complete document view. Native draws
the field caret and selected glyph spans using the actual bundled font layout;
the source caret is hidden while the field is active. Horizontal scrolling
follows the field caret. Hit proposals use that same completed glyph layout,
label offset and horizontal scroll. Dis rejects stale identities, invalid
bounds and update/end gestures without an admitted begin. Shift+click preserves
the field anchor. Focus loss, resize, zoom and mode changes retire field drags.

Ctrl+C/X/V address the active field while it is open. Copy uses only its selected
value. Cut deletes only after a successful native acknowledgement. Paste stages
the entire immutable clipboard value before single-line admission. Captured
mode and field generation supplement the document revision and transfer ticket;
unrelated input cancels pending transfers. Empty paste leaves a selection intact.
Source clipboard and its one-Undo paste behavior remain available in source mode.

## Build, example and verification

Build/install `disvm-editor` through the workspace scripts. From the installed
runtime directory, compile the tutorial using only shipped modules/interfaces:

```sh
./limbo-native -g -I include/limbo -o field-demo.dis examples/graphical-editor/field-demo.b
./disvm --jit field-demo.dis
./disvm --no-jit field-demo.dis
```

The example prints `PASS field tutorial: ЯX / atomic single-line rejection`.
It uses no native widget, document owner, clipboard service or Python runtime.
`field_test.b` checks library transitions on actual Dis. Graphical field and
clipboard tests exercise real rendering, mouse proposals, Unicode domain
transfers, strict rejected paste, resize/zoom and source Undo in both dispatch
modes. SDL dummy tests exercise its clipboard API, not physical desktop/touch
acceptance. Grapheme editing, bidirectional caret mapping and IME preedit remain
separate work; scalar editing must not be described as those features.
