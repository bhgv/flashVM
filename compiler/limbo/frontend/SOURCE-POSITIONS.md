# Normalized original-source coordinates

[Русский](SOURCE-POSITIONS.ru.md) · [Example](../examples/source-position/main.c)

`source_position.h` supplies the pure C99 `limbo_source_position` API in
`liblimbo_frontend`. It is separately named and does not reinterpret existing
Coco/R diagnostic columns or interface-binding byte columns. It can be used
without invoking the parser and from independent threads: no shared mutable state,
allocation, I/O or callbacks are involved.

The caller lends the exact original UTF-8 byte snapshot, its explicit size, a
zero-based byte boundary and output storage. Size is bounded to 2 MiB, matching
the frontend. On success the API copies the byte offset, one-based physical line
and one-based Unicode scalar column. Tab counts one scalar. CRLF is one newline;
CR and LF individually are also newlines. Combining marks count independently;
astral characters count one. No BOM removal, normalization, tab expansion,
grapheme clustering or UTF-16 conversion is performed.

The whole snapshot is validated before output publication. NUL, malformed,
overlong, surrogate or out-of-range UTF-8, internal scalar/CRLF offsets, oversized
input and invalid arguments return zero and leave output unchanged. EOF is a
valid endpoint, including the start of an empty line after a final newline.
Empty input can use a null text pointer. Unknown positions must remain explicit
in the caller's protocol; zero is a valid byte offset, not an unknown sentinel.

Our grammar tree already uses inclusive byte `begin` and exclusive byte `end`.
Keep those original half-open byte spans as conversion anchors. The new API
reports physical snapshot coordinates; Inferno `#line`/fline logical remapping
must be recorded separately. A diagnostic filename is not permission to open it.
No common Dis64 section/schema ID is introduced. File/one-based line agreement
and Unicode scalar column proposal are recorded in FLASHVM-0051; legacy private
Inferno cursor columns are unchanged.

The installed SDK includes the header, library and `examples/source-position`:

```sh
cc -std=c99 -I include examples/source-position/main.c \
    lib/frontend/liblimbo_frontend.a -o source-position
./source-position
```

A standalone frontend-only build can place the archive directly under `lib/`;
use the archive path preserved by that package. The example reports EOF at line 2, scalar column 3. It does not parse, compile or
execute Limbo. Tests independently encode every Unicode scalar and check all its
interior byte boundaries, plus mixed newlines, tabs, combining marks, astral text,
EOF, bad UTF-8 and nonmutation. Strict O3/NDEBUG UBSan and the standalone frontend
CTest suite pass. Display/IME/editor cursor policy remains a separate conversion.
