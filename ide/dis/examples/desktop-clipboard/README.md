# Session clipboard owner tutorial

[main.b](main.b) comments the complete sequence: two real pane processes, Unicode
selection, Export, acknowledged session Put, revision/range-checked Cut, Paste in
the independent target, stale generation/source rejection and immutable Clear.
It finishes by acknowledging all three owners' terminal Shutdown operations.

Run `sh run-desktop-clipboard.sh --jit`, then repeat with `--no-jit`. This is also
build-menu **12**, or `--launch clipboard`. The same example is shipped in the
editor and headless runtime packages and does not need native compiler support.

Only generated private strings are edited. The client reads/writes no file,
system clipboard or GUI. It uses ordinary channels; it does not substitute a
local mock for Idecore. Source ranges count Unicode scalars, while the transfer
limit and Reply.bytes count UTF-8 bytes. The selected π/😀 value is two scalars
and six bytes. A successful Put must precede Cut; a stale document rejection after
Put leaves the buffer copied and the source unchanged.

[Full contract](../../../DESKTOP-CLIPBOARD.md) · [Русский](README.ru.md).
