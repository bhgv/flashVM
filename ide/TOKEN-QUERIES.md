# Reuse the Coco/R catalog for context, search and lexical candidates

[Русский](TOKEN-QUERIES.ru.md) · [Lazy analysis](SYNTAX-LAZY.md) ·
[Runnable example](dis/examples/token-query/README.md) · [Entry points](ENTRYPOINTS.md)

[Idetokenquery](dis/bridge/idetokenquery.m) is a pure Limbo library over the same
completed [Idesyntax](dis/bridge/idesyntax.m) catalog used for editor colours.
[Implementation](dis/bridge/idetokenquery.b) never loads a scanner, reads a file,
changes selection/source/history or accesses FlashVM objects. One received
catalog can serve multiple queries. The UI does not run these helpers on each
keystroke; they are reusable library APIs and an explicit console example, not
a newly attached semantic-completion popup.

## Inputs and ownership

Use a completed catalog with its exact source snapshot, expected revision and
profile. Each call validates all records: quota, classes0..5, contiguous ordered
half-open scalar intervals, counters and restart extent. A source argument must
have the inferred length and contain only Unicode scalars, including ordinary
counted NUL. Equality of arbitrary same-length source contents is not authenticated;
the owner must provide the exact analyzed snapshot. All positions are Unicode
scalars, not bytes, AS UTF-16 units or pixels.

Arguments are borrowed for the synchronous call; serialize caller mutation.
No result/catalog/source is retained and no callback, channel or native handle is
used by these helpers. Context returns a value token; completion returns a new
receiver-owned string array. The strings have ordinary Limbo value semantics.
Changing a returned candidate array does not change later query results.
The analyzer separately protects retained catalogs with detached result/Snapshot
arrays; see the lazy service contract.

| API | Meaning |
| --- | --- |
| `at(catalog, revision, language, position)` | Token covering one scalar; half-open boundaries mean EOF has no invented preceding token |
| `find(catalog, revision, language, source, needle, first, kinds)` | First exact case-sensitive literal at/after first, without wrapping or selection changes |
| `complete(catalog, revision, language, source, prefix, limit)` | Sorted unique identifier spellings beginning with prefix; lexical candidates only |

Statuses: Ok0; Invalid1 for malformed/bounds/resource inputs; Conflict2 for
stale revision/profile or incomplete/error catalogs; NotFound3 for absent context
or search match. Non-Ok context has sentinel kind=-1; non-Ok find positions are
-1/-1. Completion can succeed with an empty array.

## Search and candidate policy

`find` uses a bounded KMP table and linear scalar traversal. Needle length is
1..256; first is0..source length. Bit k of the1..63 mask permits class k.
`Names` (2) admits identifier characters; `AllKinds` (63) admits every class.
Every scalar of a match must be admitted: a forbidden token resets the partial
match. A match may cross multiple allowed records. Class5 combines whitespace
and comments, so this interface cannot filter comments independently of whitespace.
Use repeated explicit calls if the document owner wants wrap-around behaviour.
No regex, Unicode normalization or case folding is implied.

`complete` considers only class1 identifiers. It excludes keyword/string/comment
records, deduplicates and sorts spellings lexicographically. Prefix is0..64
scalars, limit1..32, names over128 scalars are skipped. Sorting and bounds are
stable regardless of source encounter order. There is no type inference,
declaration resolution, scope ranking, imports or callable signature completion.
Those need the compiler semantic frontend; lexical fallback is labelled as such.

Each query validates O(tokens) records. Search adds O(source+needle), context
uses binary search after validation, and candidates use bounded insertion into
at most32 strings. These helpers are bounded synchronous operations, not a
physical latency guarantee. An interactive frontend should schedule explicit
queries outside its typing path and discard stale results by revision.

## Run and evidence

Menu **17**, `--launch tokens` or `sh run-token-query.sh` runs the actual source
example using generated scanners and the matching native-cell DisVM. Runtime
execution needs no Python/Coco/R executable or external emu. Example source and
EN/RU readmes ship in both runtime/editor packages. Menu16 remains the separate
analysis-service check; menu8 remains the graphical coloured desktop.

[idetokenquery_test.b](dis/tests/idetokenquery_test.b) checks real catalogs in
interpreter/JIT: context/EOF, Unicode, stale input, mask barriers, overlapping
literals, source extents, sorted/detached candidates and forged record rejection.
The example additionally prints actual restart/reuse counters. Document Find/
ReplaceAll also now explicitly initialize KMP prefix[0]: the existing `newa`
semantics leave scalar storage unspecified. VM allocation semantics were not
changed; the bounded helper search table explicitly initializes every entry.
