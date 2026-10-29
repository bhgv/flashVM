# Session clipboard between independent editors — v0.1677

[Desktop](DESKTOP.md) · [Pointer selection](DESKTOP-POINTER.md) ·
[Entry points](ENTRYPOINTS.md) · [Русский](DESKTOP-CLIPBOARD.ru.md)

Ctrl+C copies the focused editor's selection, Ctrl+X cuts it, Ctrl+V pastes into
its current selection/caret. The right Clipboard scroll supplies three actual
MovieClip buttons too. The buffer belongs to this IDE session: **it does not read
or write the operating-system clipboard**. The separate single-editor application
keeps its existing native clipboard integration.

![Actual MovieClip workspace with clipboard controls](assets/desktop-v0.1677.png)

The source window can close after Copy; other editors still paste the retained
immutable value. Closing the desktop terminates its clipboard process. Help and
console are read-only sources. Path fields do not yet use this session clipboard.
Copy/Cut with no selection preserves the previous buffer. Paste of an empty
buffer preserves selected source rather than deleting it.

## Owners and acknowledgements

`Idedeskclip` is a separate Limbo process with one ordinary request channel.
Get returns copied text, byte extent and a generation. Put/Clear require the
exact generation; stale or invalid writes preserve the existing value. Changed
values increment the generation without wrapping; identical values are no-ops.
Shutdown releases the retained value before its terminal acknowledgement.
Old reply strings remain independently usable after replacement or Clear.

Each editor still owns the full Idecore document, selection, saved revision and
undo. ExportSelection returns the full selected value, up to 64 KiB of UTF-8,
independently of the 200-scalar visual viewport. Only bounded presentation goes
to FlashVM. Large clipboard values stay on copied Dis channels, never in a native
UI envelope or a shared cross-VM pointer.

For Cut the application first waits for a successful clipboard Put reply, then
asks the source owner to delete the exact captured range/revision. The pane
checks both the current revision and full selection, including a changed
selection at the same revision. Paste applies one checked Idecore Replace and
collapses the private caret after the inserted Unicode scalars. Undo remains an
ordinary source operation; clipboard storage adds no document history.

This is ordered coordination, not a distributed transaction: if a source Cut is
rejected after the successful Put, the clipboard retains its copied value and
that Cut does not change the source. Direct library clients must follow the same
acknowledgement precondition; the pane does not authenticate a broker reply.

## Value and source limits

The transfer limit is 65,536 UTF-8 bytes, not Unicode characters. NUL, surrogate
values, out-of-range scalars and oversize data are rejected without partial
publication. Direct Paste clients are checked too. A failed Export cannot lead
the application into Cut. The complete document capacity remains independent.
Copy does not edit/save/build a source. Cut/Paste do not save files automatically.

Matching `.m`/`.dis` must be rebuilt and installed together: pane replies now also
carry full ordered selection bounds and an Export-only transfer string. The
native MovieClip scene format remains unchanged. No parser, SWF/Dis container,
Tk/Draw dependency or platform clipboard thread is introduced.

## Run and verify

The MovieClip IDE remains menu **8** / `--launch desktop`. The fully commented
[portable client](dis/examples/desktop-clipboard/README.md) is menu **12** /
`--launch clipboard`, or `sh run-desktop-clipboard.sh` in either runtime package.
Neither launcher needs Python, an external emu or compiler at deployment.

Acceptance runs real owners with exact/oversize byte bounds, stale generations,
immutable retained values, same-revision changed selection and empty/invalid
Paste. Actual FlashVM replay copies Unicode across independent windows, saves
complete files larger than one UI envelope, performs Cut/Undo and executes the
three MovieClip buttons. Native AMD64 interpreter/JIT are checked separately;
compiler-OFF clients also run. Physical touch, other CPU/OS and native system
clipboard execution are not inferred from these isolated session tests.
