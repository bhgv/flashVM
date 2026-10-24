# Literal bulk replacement over typed channels

[Русский](REPLACE_ALL.ru.md) · [Protocol](PROTOCOL.md) ·
[Runnable example](examples/replaceall/README.md) · [History](HISTORY.md)

Interface **0.5.0** appends ReplaceAll after Redo, preserving existing opcode/status
numbers. Request adds `replacement` immediately before `reply`; Reply appends
`matches` and `selection_end` (the latter for [Utf8Span](UTF8_SPANS.md)). Rebuild `idecore.dis`, debug information and **every client** using the
same header. Old Dis clients cannot be mixed with the new module. There is no live
ABI upgrade. Existing repository examples/tests are migrated; no Draw/Tk/UI bypass.

Send a Request with current expected revision, `[first,last)` in Unicode character
units, `text` as a nonempty literal needle and `replacement` as the insertion.
Only complete matches inside the original range count. Matching is case sensitive,
left to right, nonoverlapping, with no wrap/regex/normalization. For `aaaaa`, replacing
`aa` with `x` produces `xxa`, two matches. Inserted text is never searched.
Replacement may be empty, include the needle, LF, CR or supplementary characters.

Normal ID/revision/closed checks come first. Then range, empty-needle Invalid,
nonfitting/no-match NotFound, expansion TooLarge, identity no-op and revision cap.
No-match leaves history untouched. Equal needle/replacement returns Ok and the
match count without advancing revision or clearing Redo, even at MaxRevision.
A real change advances once, keeps saved unchanged, stores one previous snapshot
within the existing budgets and clears Redo. One Undo reverses the whole operation.

Successful ReplaceAll replies have `matches >= 1`; all other operations and rejected
requests have `matches = -1`. Navigation fields remain -1. Errors preserve text,
revision, saved and history. A nil reply channel still ignores the entire command.
The caller must receive each reply; there is no cancellation/timeout or dedup cache.

The implementation shares KMP failure links with Find. Scanning costs O(range +
needle), retaining at most floor(range/needle) hit integers plus the prefix table.
Expansion size is checked incrementally, before adding growth, to avoid overflow
from count*replacement length. Balanced rendering has logarithmic recursion depth
and O((source + output) log(matches + 1)) character-copy upper bound rather than
quadratic append per occurrence. Allocation traps belong to the runtime; no
out-of-memory recovery is promised. Logical bounds are not a total heap-byte quota.

[GC:] Source, replacement, prefix/hit arrays and intermediate strings stay rooted
within the single service owner. Completed text/history are published after
rendering; prior replies retain their immutable text. Arrays/private ADT refs never
cross the channel. This operation can occupy the owner on a large document; run
its pump outside the render/event owner. Multi-document transactions are not added.
