# Dis-owned search and file actions

[Русский](ACTIONS.ru.md) · [Controller](examples/graphical-editor/main.b) ·
[File library](bridge/idefileops.m) · [Search library](bridge/idesearch.m) ·
[Real graphical regressions](../../disVM/runtime/test_graphical_actions.py).

Ctrl+F opens a literal Find field; Enter searches from the current source scalar
and wraps at most once. F3 repeats the query. Ctrl+H uses two separate fields for
needle and replacement, so delimiters and Unicode need no ad-hoc escaping.
Empty replacement is valid; empty needle is rejected. ReplaceAll is the existing
single revision-bound owner operation and produces one Undo entry.

Ctrl+O opens a source-path field; Ctrl+Shift+S opens Save As. Ctrl+S saves the
selected path. Open checks dirty state before reading and validates the candidate
before closing the old session. Save As adopts its path and marks the exact
snapshot saved only after successful I/O. Failure preserves the previous path
and dirty state. Native partial output remains possible: writes are checked
overwrite, not atomic filesystem transactions. Invalid/NUL/oversize path and
source validation runs before native create; a malformed read is not empty success.

Prompts are Dis-owned single-line strings, limited to 256 Unicode scalars.
Committed text, scalar Backspace, field-local Ctrl+A, Enter and Escape are handled
in Dis. Prompt text never also edits the document. Enter is a key transition,
while ordinary document Enter uses the same existing channel Replace operation.
The panel is a pinned display-only TextField, controlled by View/65543 values and
committed with the viewport/snapshot at the final selection frame. The native
renderer never chooses file paths or search/replacement policy.

Idefileops supplies callable strict UTF-8 read and complete-write functions with
explicit Sys and path parameters, bounded allocation and local FD cleanup.
Idesearch supplies callable FindNext/wrap and ReplaceAll using the existing
Idecore channel, with expected revision retained across every request. Neither
library requires a window or private document pointers. Graphical Request codes
65544/65545/65546 expose Find/SaveAs/Open with flags=expected content revision;
late requests fail Conflict before changing source or destination.

Tests execute real bytecode/channels/history/native files and independently check
Unicode match ranges, wrap, two-field replacement/one Undo, empty replacement,
Open/SaveAs destination adoption, dirty rejection, failed output, counted NUL
protection and field-budget cancellation. Physical dialogs/input, regex/type-aware
search, backwards search, completion and debugger commands remain open.
