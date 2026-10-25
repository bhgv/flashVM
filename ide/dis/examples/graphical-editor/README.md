# Graphical editor: Limbo/Dis logic, FlashVM graphics

[Русский](README.ru.md) · [Controller source](main.b) ·
[Native port interface](../../bridge/idehost.m) ·
[Runtime](../../../../disVM/runtime/README.md) ·
[Architecture](../../../EDITOR-CHANNELS.md)

This is an executable first graphical editor, not a mockup. The parameterless
[`init`](main.b) runs as NC64 Dis bytecode on our interpreter/JIT. It starts the
existing [`Idecore`](../../idecore.m) document process and uses
[`Ideinput`](../../bridge/ideinput.m) for editing/selection/history policy.
[`Idebridge.sendreply`](../../bridge/idebridge.m) publishes bounded UTF-8 fragments.
The controller alone performs source-file loading/saving through Sys. Python,
Node, Inferno Draw/Tk and an external emu are not deployed runtime dependencies.

The native host registers `$Idehost` and borrows two ordinary `chan of array of
byte` ports while the Limbo entry stack roots them. Registration copies no heap
pointers into FlashVM. The owner pump uses production Dis channel movers and real
FlashVM queues. A separate FlashVM context owns a pinned TextField; only complete,
validated Dis snapshots update it. Its contours, glyph-cluster selection/caret
geometry and rasterization use FlashVM. SDL2 only presents pixels and captures
native events; it owns no document, history or file-save policy. This initial
renderer is a C embedding adapter; the existing AS presentation/worker sources
are not yet compiled and executed by this graphical target.

## Build and run

From the workspace:

```sh
# Native window SDK follows the existing explicit graphical-build preparation.
./build-workspace.sh --projects disvm-editor --jobs 4
FVM_EDITOR_FONT=/absolute/path/font.ttf sh out/disvm-editor/run-graphical-editor.sh \
    --document /absolute/path/existing-source.b
# A new destination is never read or created until the first successful save.
FVM_EDITOR_FONT=/absolute/path/font.ttf sh out/disvm-editor/run-graphical-editor.sh \
    --new /absolute/path/new-source.b
```

Menu component `disvm-editor` builds the package; post-build launch **8** requests
an explicit font and optional existing document. The font is copied by FlashVM;
no system font discovery, redistribution or synthetic fallback is introduced.
`--jit` and `--no-jit` select actual Dis dispatch modes. Module resources resolve
beside the entry module while ordinary files use the caller's working directory.
The installed package can be moved; no source checkout is needed to execute it.

Direct CMake build, with an explicitly supplied SDL2 SDK:

```sh
cmake -S disVM/runtime -B build/editor -DDVM_RUNTIME_EDITOR=ON \
    -DDVM_EDITOR_SDL=ON -DSDL2_DIR=/absolute/path/SDL2/cmake \
    -DDVM_EDITOR_TEST_FONT=/absolute/path/font.ttf
cmake --build build/editor -j4
ctest --test-dir build/editor --output-on-failure
build/editor/disvm-editor --font /absolute/path/font.ttf \
    --new example.b build/editor/graphical-editor.dis
```

The console/runtime targets remain buildable without this option or SDL/fonts.
`DVM_RUNTIME_EDITOR=ON` without SDL supports deterministic offscreen replay and
real FlashVM raster output. `DVM_EDITOR_TEST_FONT` explicitly enables the graphical
CTest cases; an absent font does not count as a passed graphical test.

## Interaction and files

| Input | Dis-owned behavior |
| --- | --- |
| Native text/IME, Enter, Tab | Insert committed Unicode text through the document channel. |
| Left/Right, Home/End, Shift | Move or extend selection in Unicode scalar coordinates. |
| Backspace/Delete, Ctrl+A | Delete an acknowledged range or select the document. |
| Ctrl+Z, Ctrl+Shift+Z, Ctrl+Y | Use the document owner's Undo/Redo history. |
| Ctrl+S | Write UTF-8 to the configured path; mark only the written revision saved. |
| Ctrl+O | Reload the configured path only when the current document is clean. |
| Close window | Stop cleanly, or refuse closure with status Conflict when dirty. |
| Shift + close window | Explicitly discard changes and stop. |

The title shows revision, saved revision and status (`0` success, `1` conflict,
`4` invalid/I/O failure). The controller sends all file operations through Sys;
initial loading rejects malformed UTF-8 and files over 64 KiB. An initial read
failure is terminal and never creates/truncates a file. Ordinary typing remains
bounded by Idecore. Save overwrites the selected native file, handles short
writes and acknowledges only success. It is not an atomic replacement or a
power-loss durability guarantee; a failed write can leave partial disk content.
`--new PATH` is explicit permission to replace that path when saving. Without a
path, a new buffer can be edited, but Save reports an error. No Save As dialog yet.

Focus loss resets held modifiers in Dis and FlashVM. A native event is consumed
only after its predecessor's complete view; buffered transport owns each accepted
copy until delivery. Quit stops the document service, then calls `detach` before
the controller's stack can release the borrowed channel roots. Both VM heaps,
collectors and execution state remain separate on one native owner.

## Validation and limits

[`test_graphical_editor.py`](../../../../disVM/runtime/test_graphical_editor.py)
constructs independent wire envelopes. Its replay files contain a four-byte
little-endian frame length followed by one `fvd1:` envelope; length is checked
before copying. It runs actual compiled Limbo code, production rendezvous/GC,
FlashVM TextField and raster output, checking Unicode offsets, Undo/Redo, dirty
close/reload, save bytes, invalid UTF-8, oversize admission, snapshot fragmentation,
relocated cwd and real dispatch counters. `--events FILE` is explicitly offscreen;
`--capture FILE.ppm` records real pixels. EOF without acknowledged Quit is failure.
`--window-events FILE` additionally uploads those pixels through a real SDL window;
CTest uses SDL's dummy driver for presentation protocol checks. Those checks do
not establish physical window, keyboard/IME or device acceptance.

Current acceptance is Linux AMD64, experimental NC64, one native owner. The
viewport is fixed at 960×640; the first 4096 UTF-8 bytes are previewed without
splitting a scalar. The full document is still owned/saved by Dis. Scrolling,
Up/Down navigation, mouse hit-testing, mixed-color syntax, completion popups,
AS-worker launch and compiler/debugger commands remain open. Selection/caret use
glyph clusters; full bidi/grapheme editor interaction is not claimed. Font glyph
coverage depends on the selected face. Standard-input/file/DNS calls may still
block the single native owner. Full portable reusable embedding and multiple VM
owners are separate work.

## Viewport / Окно документа v0.1552

[English contract](../../VIEWPORT.md) · [Русский](../../VIEWPORT.ru.md).
Wheel, mouse click/Shift/drag, Up/Down/PageUp/PageDown and Ctrl+Home/End now run
through the Dis controller. The old first-prefix/unfinished-mouse limitations
above describe v0.1550 only. Windows are Dis-selected, bounded UTF-8 intervals;
full text remains in the document owner. Physical device acceptance stays open.
