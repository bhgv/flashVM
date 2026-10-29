# MovieClip desktop entry tutorial

`main.b` is a fully commented typed launcher for the real `desktop-ide.dis`
application. It retains the loaded module until all window/layout processes
acknowledge shutdown. No source compiler, external emu, Python or HTML UI runs.

From a matching installed graphical SDK:

```sh
./disvm-editor --jit --document example.b desktop-example.dis
./run-desktop-ide.sh --new another.b
```

Physical windows need an SDL-enabled host. Headless acceptance drives actual
MovieClip rendering with --events/--capture. The independently owned panes start
with two editors, build console and Help. Click to focus, use left live miniatures
to navigate, drag empty table space to scroll, and expand right tools. F1 opens
Help, Ctrl+N creates an editor, Ctrl+O opens, Ctrl+S saves, F7 compiles, F8 selects
a source diagnostic. Read the complete [desktop contract](../../../DESKTOP.md).

This first shell uses bounded caret-following text viewports and keyboard editing.
Clipboard, off-pane drag capture, graphics and attached debugger sessions are still
being integrated. A Debug scroll explicitly reports unavailable session state.
Save writes only an explicit focused editor destination; Open protects dirty text.
Quit requires explicit discard for unsaved editors. Merely starting the example
creates no source file and never loads/runs a compilation artifact.

Drag a title to move its window; `x`/Ctrl+W closes only that editor. Dirty text
requires explicit discard; Cancel/Escape keeps it. Empty space still pans the table.
[Details](../../../DESKTOP-LIFECYCLE.md) · [Portable example](../desktop-lifecycle/README.md).

[Mouse caret and selection](../../../DESKTOP-POINTER.md).
