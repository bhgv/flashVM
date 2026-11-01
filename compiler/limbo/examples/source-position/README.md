# Physical UTF-8 coordinates and spans

This C99 example calls `limbo_source_position` and `limbo_source_span` on one
immutable snapshot, without loading a parser or opening source files. It supports
IDE/debug consumers converting the Coco/R tree's original byte anchors.

The bytes contain a tab, lambda, astral smile, CRLF, e and a combining accent.
The second line begins at byte9; EOF is byte12, physical line2, scalar column3.
Tabs count one, CRLF counts one newline, and the combining accent counts one
separate scalar. The range9..12 is half-open. Both span endpoints are resolved
in a single complete validation pass; invalid trailing bytes still refuse.

From an installed Limbo frontend SDK:

```sh
cc -std=c99 -I include examples/source-position/main.c lib/frontend/liblimbo_frontend.a -o source-position
./source-position
```

Use the actual archive path under `lib/` if your standalone package preserves a
different directory layout. Expected output:

```text
EOF byte=12 line=2 scalar-column=3
span=9..12 line=2 columns=1..3
```

All storage is borrowed/caller-owned; neither function allocates, blocks, accesses
files or shares parser state. On failure the supplied output is unchanged.
Do not confuse scalar columns with UTF-16, display cells, graphemes or legacy
scanner diagnostic counters. Logical `#line` identities stay separate. A filename
or coordinate is no authority to open a file. This example does not emit a Dis64
debug section or execute compiled code.

[API contract](../../frontend/SOURCE-POSITIONS.md) ·
[Declarations](../../frontend/source_position.h) ·
[Implementation](../../frontend/source_position.c).
