# Revision-bound Undo and Redo

[Русский](HISTORY.ru.md) · [Protocol](PROTOCOL.md) ·
[Implementation](idecore.b) · [Runnable example](examples/history/README.md) ·
[Compiler diagnostics](../../fir/v1/doc/SOURCE_DIAGNOSTICS.md)

IDE 0.4.0 appends Undo/Redo opcodes after Find without changing the existing
Request/Reply layouts or older opcode values. Rebuild the core and clients with
the same [idecore.m](idecore.m). All edits and history travel go through the
same typed request channel; the private Document/History ADTs are not exposed.
This implements an editor document service, not a filesystem or graphical editor.

## Observable behavior

Undo restores the most recent prior text, pushing the current text onto Redo.
Redo performs the inverse. A successful travel increments revision exactly once;
it never restores an old revision identity. For example, Open first at revision 1,
Replace second at revision 2, Undo first at revision 3, Redo second at revision 4.
A command carrying expected=2 after Undo receives Conflict even though the first
text has appeared before. Revision identities are service-lifetime values.

An actual Replace pushes the previous text and clears Redo. A no-op Replace,
rejected request, Find/Locate/Offset query or MarkSaved does not discard Redo.
MarkSaved does not clear Undo either. Empty text is a valid snapshot.
Open/Close clear both stacks; history never crosses a retired document session.
Shutdown stops the owner after its acknowledgement; runtime GC may then reclaim
private roots when no process retains them.

Saved is a revision acknowledged by a real external save. Undo/Redo never change
it, even if restored text happens to match the spelling previously saved.
Dirty state remains conservative (`revision != saved`). Clients may acknowledge
an actual completed save with MarkSaved at the expected current revision;
text equality alone is not a filesystem save acknowledgement.

The unused first/last/text fields are ignored by Undo/Redo. A reply carries the
complete current snapshot, echoed ID, and offset/line/column=-1. Error precedence:

1. Nil reply requests are ignored; negative IDs are Invalid.
2. Unknown opcode is Unknown; stale expected revision is Conflict.
3. Closed documents reject Undo/Redo with Closed.
4. Empty requested history is NotFound, including at the revision maximum.
5. A nonempty stack at MaxRevision rejects travel with RevisionLimit.

All rejected commands preserve text, revision, saved and both stacks. The revision
cap never wraps and failed travel never consumes a history entry. Request/reply
backpressure and mandatory reply ownership are unchanged; there is no cancel,
transaction replay cache or subscriber broadcast.

## Budgets and eviction

Production limits from idecore.m apply to the combined Undo+Redo stacks:

| Limit | Value | Unit |
| --- | --- | --- |
| MaxHistoryEntries | 64 | Snapshots, including empty strings |
| MaxHistoryCharacters | 1048576 | Sum of snapshot character lengths |
| MaxCharacters | 1048576 | Current document, separate from history |

The stacks are persistent lists, newest first. Each edit/travel prepares its new
text and bounded candidate History before publishing document fields. Travel
first pops the source stack and pushes the inverse snapshot onto its target.
When over budget, evict the farthest source snapshots first, then the farthest
target snapshots. For a Replace, source is empty, target is Undo and Redo is
cleared. This retains the newest inverse snapshot whenever it fits.
An oversized snapshot relative to a test/custom history budget can be evicted;
that does not reject an otherwise valid edit.

These are logical character/entry bounds, not measured heap bytes. The current
text, UTF-8 representations, list nodes, retained client replies and transient
concatenation allocations have separate runtime lifetimes. Old Reply text values
remain valid. Allocation/runtime traps are not caught or promised atomic.
The list rebuild algorithm is quadratic in the bounded entry count (at most 65
candidate entries), not an unbounded document history traversal. Character sums
remain comfortably inside the signed-int range at production limits.

## Verify and run

Use explicit native Limbo, reference emu and module SDK paths from the
[first-steps guide](../../README-FIRSTSTEPS.md):

```sh
python3 build-menu.py --projects ide --tests --demo history
```

The example prints first restored at revision 3/saved 1, third on a branch at
revision 6/saved 1, and a PASS marker. CTest requires both a successful process
exit and the marker in reference modes 0 and 1. Modes are not evidence of new
native JIT backend entry.

`history_test.b` covers Unicode, stale requests, nil replies, no-op/rejected edits,
saved state, branching, retained replies, sessions, exactly 65 edits and 64
undo/redo steps. `historylimits_test.b` compiles unchanged production source with
private test-only limits of three entries and six characters to exercise exact
boundaries, source-first eviction, empty and oversized snapshots. That fixture
is never installed. `revision_test.b` exercises terminal guards with MaxRevision=4.

The product still installs only idecore.dis, idecore.sbl and idecore.m. The external
reference runtime caveat in [REFERENCE_RUNTIME.md](REFERENCE_RUNTIME.md) remains;
this series does not fix or import that runtime, embed Dis into FlashVM or accept
the target JITs/debugger. History is in-memory per service, without persistence,
coalescing, multi-document management or an allocation-failure recovery protocol.
