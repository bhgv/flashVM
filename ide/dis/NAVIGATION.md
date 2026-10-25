# Dis-owned logical-line navigation

[Русский](NAVIGATION.ru.md) · [Policy interface](bridge/ideinput.m) ·
[Implementation](bridge/ideinput.b) · [Executable regression](tests/navigation_test.b).

`Ideinput.apply` now handles Up/Down, PageUp/PageDown and Ctrl+Home/End alongside
existing horizontal movement, selection and history. All commands first obtain
an acknowledged document snapshot through the existing typed channel. Navigation
never changes text, content revision or Undo/Redo history.

Repeated vertical commands retain a preferred Unicode scalar column. A short
line clamps the current caret but does not erase the goal, so returning to a
longer line restores its column. Horizontal moves, Home/End, edits/history and
external revision changes reset that goal. Shift extends the existing anchor;
Ctrl+Home/End moves to document boundaries. Page movement uses 24 logical lines
by default, with an admitted state range of 1..128. Invalid page configuration
falls back to 24 rather than enabling an unbounded loop.

Logical lines use LF. A CR immediately before LF or EOF is excluded from End
and vertical destination columns, while remaining in the unchanged document.
Coordinates count Unicode scalars, not UTF-8 bytes or graphemes. Soft-wrapped
visual-line movement and bidi/grapheme editing are not claimed.

The native SDL adapter translates actual key transitions into the existing value
protocol; document decisions stay in Dis. `State` gained preferred/page fields,
so rebuild every client with the matching `.m`. The regression executes actual
Dis document channels and verifies uneven Unicode lines, retained goals, paging,
Shift selections, CRLF, unchanged history and stale external revisions in both
interpreter/JIT modes.
