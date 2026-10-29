# Glyph caret and selection — v0.1675

[Desktop](DESKTOP.md) · [Window lifetime](DESKTOP-LIFECYCLE.md) ·
[Русский](DESKTOP-POINTER.ru.md) · [Portable example](dis/examples/desktop-pointer/README.md)

Click an editor's text body to place its caret. Shift-click extends its existing
anchor; drag within that body to select. Typing replaces the complete admitted
source selection through the existing Ideinput/Idecore path, with ordinary undo.
The focused field and its miniature show genuine MovieClip selection rectangles
beneath the text. Keyboard selection is displayed by the same path too.

![Actual glyph selection](assets/desktop-v0.1675.png)

Clicking another editor's body activates it without jumping the table. Titles
still move windows, empty table space still scrolls the table, and title close
buttons retain their separate guarded lifetime. Help and console are not editable.

## Three separate admissions

1. The FlashVM owner measures the actual completed MovieClip matrix and the same
   font/wrap/tab glyph layout used by caret and selection drawing. It proposes a
   stable widget ID and a Unicode insertion offset in that copied TextField.
   Topmost overlays, rails, headers and close buttons do not become text hits.
2. Limbo layout independently admits the body identity against the current view
   ticket and the same overlap order. `Activate` changes focus without scrolling.
   Native glyph metadata never selects an arbitrary source owner by itself.
3. The pane maps the visible offset into its complete source and checks the exact
   source revision/range. Its private Ideinput state owns the full anchor/caret.
   A new snapshot also prevents a previous rejected command's status from blocking
   a valid unchanged-revision selection. Selection changes no source revision,
   saved revision, file, compilation result or undo history.

The root caches each shown source revision, visible first offset and caption
prefix for that complete scene. UTF-8 byte clusters are converted to Unicode
scalar offsets; caption bytes are not source coordinates. The service's endpoint
remains a full document offset even when only a clipped selection is displayed.
No native object, font pointer or managed address crosses into Dis.

## Gesture cancellation

Begin/move/end are ordered 1/2/3. Begin captures a private anchor, or keeps the
existing anchor for Shift. A move/end without an admitted begin is rejected.
Stale source revision, stale view, malformed metadata, focus loss, another input,
modal decision, wheel/resize or leaving that body cancels continuation. A late
release cannot revive it. Cancellation preserves the current source/selection;
it does not synthesize an edit or roll back already admitted endpoints.

The first version selects inside the currently visible body. It does not yet
capture a pointer across other panes or auto-scroll beyond the body. Logical
selection is scalar-based; shaping clusters govern the visual rectangles/hits,
not a complete grapheme-navigation or bidi-affinity implementation. Newline glyphs
have no extra full-line highlight rectangle. Clipboard integration remains work.

## Copied protocol

Matching `.m`, `.dis` and native runtime must be rebuilt/installed together.
The desktop Item now carries nine hexadecimal words: parent, x, y, width, height,
style, caret, mirror, anchor, followed by counted UTF-8. Hidden endpoints are
`ffffffff`. A native pointer proposal carries four words: phase, button, widget,
TextField scalar offset; absent hits are widget zero/offset `ffffffff`.

`Idedeskpane.Place` uses a copied Uibridge Request: code=phase, flags=exact source
revision, first=absolute scalar offset, second=Shift 0/1. `CancelPointer` drops only
the gesture. Reply anchor is clipped to the visible body; full source selection
stays private. Layout `TextHit` verifies body/ticket and never edits source.
These are bounded channel values, not new SWF/Dis container tags or a parser.

Native painting/measurement stays on the FlashVM owner. Only completed pixels go
to detached presentation. Separate Limbo processes remain cooperative. Tests run
actual AMD64 interpreter/JIT, glyph readback/raster, Unicode/CRLF, moved windows,
independent activation, dirty/modal protection and stale/cancel cases. Physical
touch and other OS/CPU acceptance are separate tasks.
