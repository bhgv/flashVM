# Dis source editing and locations

[Русский](EDITING.ru.md) · [Controller](examples/graphical-editor/main.b) ·
[Reusable policy](bridge/ideinput.m) · [Application tests](tests/words_test.b) ·
[Indent tests](tests/indent_test.b).

Ctrl+Left/Right moves by scalar word runs; Shift extends the selection.
Ctrl+Backspace/Delete removes the preceding/following run through one checked
Replace and one Undo entry. A nonempty selection is removed as-is. A movement
without Shift first collapses an existing selection to its corresponding edge.
Boundary no-ops do not create history. Focus loss clears modifiers.

Runs are explicit: Unicode spacing separates words, ASCII letters/digits/_ and
other non-ASCII scalars form text runs, and other ASCII punctuation forms its
own runs. This deterministic editor policy is not Unicode UAX #29 segmentation,
full grapheme navigation or language tokenization. CRLF is one ordinary
Left/Right/Backspace/Delete step; the file's original separators are preserved.

Tab at a collapsed caret inserts spaces up to the next scalar column divisible
by four. Tab on a selection adds four spaces to each affected logical line.
Shift+Tab removes one leading tab or up to four leading spaces. A selection
ending at the next line start excludes that line. Block selections expand to
the complete affected lines, retain direction, and change source in one Replace.
A collapsed unindent preserves the caret's position relative to the remaining
text. A no-op unindent preserves revision/history. Fragments are joined with
balanced concatenation and the result is bounded before source publication.

Ctrl+G opens a Dis-owned `line[:column]` field. Both coordinates are positive,
one-based decimal; the optional column defaults to 1 and counts Unicode scalars.
Long columns clamp at logical EOL, excluding CR before LF. A final LF creates a
valid final empty line. Missing lines, zero, signs, spaces, extra separators and
integer overflow are rejected, leaving the field open for correction; Escape
cancels. A successful jump collapses selection and reveals the caret without
changing document, saved revision or history.

Embedding clients can send revision-bound Request/65551 with line/column in
first/second and empty text. It calls the same `Ideinput.locate` and checked
selection policy as the field. Stale revision returns Conflict; invalid location
returns RangeError; nonempty request text returns Invalid. No native pointer or
source edit crosses this boundary. Existing search/path prompts retain their
own input policy and do not treat these source commands as text.
