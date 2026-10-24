# Compiler diagnostic spans in the Limbo editor service

[Русский](UTF8_SPANS.ru.md) · [Protocol](PROTOCOL.md) ·
[Real compiler example](examples/compiler-location/README.md)

Interface 0.5.0 appends `Utf8Span` after ReplaceAll. Request `first,last` are a
half-open byte range in the **original UTF-8 encoding of the retained source**.
Both endpoints must be Unicode scalar boundaries. Empty ranges, including EOF,
are insertion points. Request text/replacement are ignored. Expected revision,
nonnegative ID, open state and reply rendezvous follow the normal protocol.

A successful reply contains character offset/selection_end and the start's
one-based LF-delimited line/zero-based character column. Other operations and
errors leave selection_end=-1. This is another 0.5.0 header change: rebuild every
client, core and symbols together. Existing operation/status numbers are preserved.

The service validates every scalar in the snapshot and counts canonical UTF-8
widths (1/2/3/4). Surrogates, negative values and values above U+10FFFF produce Invalid,
even when outside the selected range: such a Limbo string has no canonical UTF-8
source snapshot. Negative/reversed input ranges are RangeError before validation;
non-boundary/out-of-file endpoints are RangeError after validation. No replacement
character or rounding is applied. U+0000 is a scalar; a compiler can independently
reject NUL in source. BOM counts as U+FEFF, three bytes and one editor character.
CR counts as a character, LF alone begins a new line; combining marks remain
separate characters. No UTF-16/grapheme/display-column conversion is implied.

Read-only conversion needs O(document characters) time, at most one additional
prefix scan for line/column and constant extra storage. Maximum byte count is
4*MaxCharacters, within a signed 32-bit int. It does not mutate text/history/saved
and remains available at the terminal revision. No native pointers or heap roots
are transmitted. All requests still occupy the sole document owner until replied.

For FIR JSON diagnostics, use byte_offset and byte_offset+byte_length **only if
has_location=true** and the source/revision still match the submitted snapshot.
Check the addition/range before constructing a Limbo int. A diagnostic from another
file or an older revision must never be applied to current text. Conflict rejects
a stale request; the caller must also track the service/document identity because
independent service instances can share revision numbers. Resource diagnostics
without a location should be shown without selecting text.

This maps original source diagnostics and FIRDBG1 source events. It is not final
Dis/SWF/native-PC relocation, a debugger breakpoint mapper or a compiler process
inside the deployed IDE. The optional integration invokes the real native FIR
compiler as host test tooling, then runs the commented Limbo client in an explicit
matching reference VM. No deployed Python dependency is added.
