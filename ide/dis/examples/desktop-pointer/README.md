# Portable pointer-selection owner tutorial

[main.b](main.b) comments every part of a real pane-service session. It starts a
private document containing `aπ😀z`, admits begin/move/end at its current revision,
selects two Unicode scalars, replaces them through Ideinput and rejects an old
revision without moving the caret. It finishes with acknowledged Shutdown.

Run `./disvm --jit desktop-pointer.dis`, then `--no-jit`. The installed runtime
ships matching `.m`/`.dis` dependencies. Native compiler support is optional.
The generated source exists only in the example's private document; no file is
read/written and no compilation is invoked.

This is a typed service client, not a GUI or a glyph-width approximation. It
supplies explicit source proposals. The actual graphical desktop additionally
checks real FlashVM glyph geometry, view ticket, layout identity and caption/
viewport mapping. Success here does not substitute physical mouse/touch tests.

Positions count Unicode scalars, not UTF-8 bytes. Selection does not add undo
history or change revision. Replacement is one actual owner edit. Stale proposals
cancel continuation and preserve source/selection. Never reuse the endpoint after
its terminal Shutdown reply.

[Full pointer contract](../../../DESKTOP-POINTER.md) · [Русский](README.ru.md).
