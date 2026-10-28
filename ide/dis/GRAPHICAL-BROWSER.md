# Graphical directory browser — v0.1653

[Русский](GRAPHICAL-BROWSER.ru.md) · [Model interface](bridge/idebrowser.m) ·
[Model implementation](bridge/idebrowser.b) · [Dis controller](examples/graphical-editor/main.b) ·
[Directory owner](DIRECTORIES.md) · [Tutorial](examples/directory-browser/README.md)

The browser displays up to eight native entries at once, with directory/file
markers, selected-row highlighting and a clickable toolbar. Ctrl+B opens the
path field; Enter loads a batch of up to 32 entries. Selection automatically
moves the visible eight-row window. Source history remains independent.

The controller and browser owner are **Limbo**. The shipped
[AS source](../as/editor-view.as) is compiled by our
[AS2 CLI/library](../../compiler/as2/view/README.md) through FIR → SWO → SWL → SWF and actually executed as AVM1
in FlashVM for each complete view. It assigns TextField content and widget
geometry. The native adapter still paints fonts/ranges, supplies bounded surface
metrics and maps completed glyph positions; the complete legacy editor renderer
has **not** yet been migrated to AS. No Inferno GUI or native file dialog is used.

| Input while browsing | Result |
| --- | --- |
| Click row | Select that visible entry without opening or editing it. |
| Click Open | Descend/open the selected entry under ordinary dirty-source checks. |
| Up / Down | Clamp selection within the current batch. |
| Backspace / Up toolbar | Open the parent directory. |
| Page Down / Next toolbar | Request the next native batch; EOF keeps the last entries selectable. |
| Enter | Descend into a directory or request loading the selected file. |
| B / Ctrl+B | Edit the directory path, preserving the current cursor until admission succeeds. |
| R / Refresh toolbar | Refresh from the first batch of the current path. |
| Escape / Close toolbar | Close the cursor and return to document input. |

The path field uses the existing Unicode field policy: 256 scalars. Escape there
returns to an existing browser, or the document if no directory is open. Native
order is preserved. Previous-batch history, sorting, expandable trees, drag/drop,
standard icon columns and timed double-click are still absent: this is a basic
listing, not a complete desktop file manager. Mouse selection and the Open toolbar
are implemented. Text/paste/delete never become document edits while browsing.
Resize, zoom and window
close continue through their existing checked channels.

## Ownership and admission

`Idebrowser.create(files, sys)` returns an exclusive controller-owned `ref State`
and starts one Idefiles owner. Never share this mutable state with another process
or native renderer. `open` stages the candidate FD and first batch before closing
the old cursor: missing paths, ordinary files and first-batch errors preserve the
old path and selection. `advance` replaces only complete batches. Native failure
retires the cursor and its entry cache; EOF retains the last page and releases
the native FD through Idefiles. `move` accepts only -1/+1 and never wraps.

`choose` descends into a selected directory or returns PickFile with the **full**
joined path. It performs no document load itself. The graphical controller checks
the current document's dirty state, reads/validates the actual source, and waits
for Close/Open acknowledgements before adopting the new save destination. Dirty
or failed reads keep the document and browser available. Successful loading
resets the normal document selection/history session and closes the browser.
File metadata is an observation, not a filesystem lock: a changed or removed file
is checked by the subsequent real load, not trusted from its listing record.

All clean/forced application close paths call `shutdown` before native channel
borrowing ends. Shutdown is acknowledged once; repeated calls send no request to
a dead endpoint. Request IDs never wrap and the last ID is reserved for terminal
cleanup. Candidate open reserves room for admission, first read and close replies,
so near-exhaustion cannot strand a newly opened cursor. Directory identity remains
endpoint-local, as documented by Idefiles; this is not a network lease protocol.

`summary` returns a copied control-escaped multiline UTF-8 listing below 1024 bytes.
The model computes scalar row ranges and resolves toolbar/row actions; native
geometry proposes a scalar only. FieldPlace requires the exact revision/view
ticket and press phase; stale proposals cannot select or open anything. Only previews are abbreviated; path/name action values are
never truncated. The model supports the Idefiles 4096-byte path bound; graphical
path fields are limited to 256 scalars and callable envelopes to 1024 UTF-8 bytes.
Ordinary slash path joins are passed to OS ports without canonicalization; native
Windows/macOS/Android acceptance remains open.

## Callable graphical requests

[Idehost declarations](bridge/idehost.m) use ordinary Uibridge Request envelopes.
All five operations require flags=current document revision and second=current
completed-view ticket. A stale ticket/revision returns Idecore.Conflict without
moving selection or loading a file. Invalid payloads return Idecore.Invalid.
Each completed view advances the ticket even when the document is unchanged.

| Code | Name | first / text |
| --- | --- | --- |
| 65564 | Browse | 0 / directory path |
| 65565 | BrowseMove | -1 or +1 / empty |
| 65566 | BrowseNext | 0 / empty |
| 65567 | BrowseChoose | 0 / empty |
| 65568 | BrowseClose | 0 / empty |

Only Browse can begin outside the browser; the other requests require browser
mode. The native renderer accepts Panel 9 (read-only browser) and 10 (directory
path field with normal caret/selection). It receives bounded copied text, never
Sys.Dir arrays, native FDs or policy callbacks. All UI decisions remain in Dis.

## Verification and limits

Real replay tests run both interpreter/JIT modes, inspect actual raster changes,
select/load/save Unicode files, navigate subdirectories, page 40 entries, reject
stale tickets, preserve dirty source and retain the old cursor on failed replacement.
The tutorial additionally checks model shutdown cycles, selection bounds, label
size and request-identity exhaustion under quantum 1/2048. Standalone native and
classic compiler checks use explicit external headers; classic compilation alone
is not classic-runtime acceptance.

Native I/O still blocks the single VM owner. Graphical replay is Linux AMD64
acceptance, not physical window/touch acceptance on every OS. No asynchronous
cancellation, `/prog` debugger, remote cursor lifetime or complete IDE compiler
self-hosting is introduced here. The source-built legacy Limbo remains a bootstrap;
our Coco/R compiler remains a separate development track.
