# Caret, text and mouse share the same layout contract

[Русский](CARET.ru.md) · [Native renderer](../../disVM/runtime/editor_view.c) ·
[Independent raster regression](../../disVM/runtime/test_graphical_caret.py).

The former overlay used the font's natural `line_height` while the actual
TextField used `ascent + descent + leading`. With Noto 11 the mismatch accumulated:
on the tenth line the caret's top was at y=137 while that line's actual ink was
at y=168..175. The same mismatch affected mouse line hits and visible-row counts.
This was a presentation defect; the Dis source offsets themselves remained valid.

The editor explicitly sets leading=2 and kerning=false on its TextFormats and
uses precisely those settings in the separate owned hit layout. The layout
baseline step, caret/selection rectangles, mouse hits, page capacity and prompt
reserve all use `ascent + descent + 2`. Existing input tabs use the same periodic
four-average-character-width stops as AS2 TextField, and hit layout shaping also
disables kerning. No font substitution or global TextField semantic change is
introduced. The pinned footer is excluded from source hit testing.

The regression independently scans actual rendered RGB pixels into text ink
rows and compares the blue caret against the tenth row of a twelve-row source.
It then clicks coordinates derived from those ink pixels and checks the Dis
source selection. It covers normal text, real tab characters, AV pairs, sizes
9/11/14 and interpreter/JIT execution. Rebuilding the parent renderer against the
same controller fails this test at the first tenth-line comparison; the fixed
renderer passes. Tests do not merely compare two copies of the new formula.

This does not add full grapheme/bidi caret navigation, blink timers or physical
input-device certification. Those remain separate from the repaired geometry.
