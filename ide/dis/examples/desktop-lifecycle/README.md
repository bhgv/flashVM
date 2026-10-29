# Independent owner retirement tutorial

[main.b](main.b) comments the complete sequence: start two real pane processes,
edit one through its input channel, observe the untouched peer, publish layout
values, consume terminal Shutdown, remove that presentation and create a fresh
owner. It uses its own generated strings; no file is read, written or compiled.

Run `./disvm --jit desktop-lifecycle.dis`, then repeat with `--no-jit`. The matching
runtime ships the module and all dependencies. This is a portable service client,
not a graphical mockup: it does not attach FlashVM or emulate a confirmation UI.
The actual desktop application handles interactive dirty-close decisions.

Each helper waits on one private acknowledgement. After Shutdown the old channel
is terminal and is never used again. A new start creates a new Idecore document;
its clean initial revision is independent of the peer's source and old undo.
Layout Remove follows document shutdown, and cannot remove system slots 2/3.
Every failure raises explicitly; the PASS line follows acknowledged cleanup.

[Complete lifecycle guide](../../../DESKTOP-LIFECYCLE.md) ·
[Graphical entry example](../desktop/README.md) · [Русский](README.ru.md).
