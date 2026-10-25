# Graphical editor: Limbo/Dis logic, FlashVM graphics

[Русский](README.ru.md) · [Fully commented controller](main.b) ·
[Host interface](../../bridge/idehost.m) · [Runtime](../../../../disVM/runtime/README.md) ·
[Navigation](../../NAVIGATION.md) · [Viewport/mouse](../../VIEWPORT.md) ·
[Search/files](../../ACTIONS.md) · [Font/zoom](../../ZOOM.md).

The executable controller runs as real NC64 Dis bytecode on our interpreter/JIT.
Idecore owns source/history in a separate channel process; Ideinput owns cursor,
selection and navigation. Ideviewport chooses a bounded Unicode display window.
Reusable Idefileops and Idesearch provide source I/O and document actions without
a renderer dependency. The application, fields, dirty-state and zoom policy are
Limbo. Python/Node, Inferno Draw/Tk and external emu are not deployed dependencies.

A separately owned FlashVM context displays pinned TextFields. Native code only
renders complete checked channel values and converts actual glyph geometry to
revision/ticket-bound hit proposals; Dis decides whether to apply them. SDL
presents pixels and captures events. The existing AS worker/presentation sources
are not yet executed by this target.

## Build and run

```sh
./build-workspace.sh --projects disvm-editor --jobs 4
sh out/disvm-editor/run-graphical-editor.sh --new example.b
# Open an existing strict UTF-8 source instead:
sh out/disvm-editor/run-graphical-editor.sh --document existing.b
```

The root menu builds `disvm-editor`; post-build launch **8** starts it. Bundled
OFL Noto Sans Mono starts at **11 pixels**, with no font-location question or
system font search. Its face/license/provenance are shipped in `fonts/`.
`--font PATH` / `FVM_EDITOR_FONT` is an optional override. The installed package
can be moved; module/font resources resolve beside the entry module while ordinary
file paths retain the caller's cwd. No network/font download occurs at execution.

Direct CMake builds use `DVM_RUNTIME_EDITOR=ON`; SDL windows additionally use
`DVM_EDITOR_SDL=ON` and an explicit SDL2 SDK. The console runtime remains independent
of graphics. Without SDL, the graphical target still supports deterministic
replay with actual FlashVM raster output. Matching Dis/.sbl modules are built from
source. `--jit` / `--no-jit` selects real Dis dispatch.

```sh
cmake -S disVM/runtime -B build/editor -DDVM_RUNTIME_EDITOR=ON \
    -DDVM_EDITOR_SDL=ON -DSDL2_DIR=/absolute/path/SDL2/cmake
cmake --build build/editor -j4
ctest --test-dir build/editor --output-on-failure
build/editor/disvm-editor --new example.b build/editor/graphical-editor.dis
```

## Interaction

| Input | Dis-owned effect |
| --- | --- |
| Native text/IME, Enter, Tab | Insert committed Unicode through the document channel. |
| Left/Right, Up/Down, Home/End, Shift | Navigate/select source scalars; vertical moves retain their column across short lines. |
| PageUp/PageDown, Ctrl+Home/End | Move by current visible logical rows or document boundary. |
| Mouse, Shift+click, drag | Propose actual glyph positions; Dis checks the current revision/view ticket. |
| Wheel | Scroll logical lines without editing source or moving the caret. |
| Ctrl+wheel, two-finger pinch | Zoom in/out, bounded 6..48; default 11, actual row capacity follows the face. |
| Backspace/Delete, Ctrl+A | Delete the acknowledged range or select the source. |
| Ctrl+Z, Ctrl+Shift+Z, Ctrl+Y | Actual document Undo/Redo. |
| Ctrl+F, F3 | Literal Find field, then repeat/wrap once. |
| Ctrl+H | Separate literal needle/replacement fields; ReplaceAll is one Undo entry. |
| Ctrl+O | Open a strict UTF-8 path after checking the current document is clean. |
| Ctrl+S, Ctrl+Shift+S | Save selected path, or choose Save As and adopt it only after successful output. |
| Field Ctrl+A, Backspace, Enter, Escape | Select/edit/accept/cancel the field without also editing source. |
| Close / Shift+close | Refuse dirty closure / explicitly discard changes and stop. |

The title reports revision, saved revision, status and logical top line. Status
0 is success, 1 conflict, 2 range, 3 too large, 4 invalid/I/O and 8 not found.
Initial/Open source reads are limited to 64 KiB and reject malformed UTF-8 before
retiring the old document. The file library itself has a bounded 4 MiB API for
other callers. `--new PATH` selects a destination without touching it until Save;
a pathless new buffer can use Save As. Successful save acknowledges the exact
snapshot written. Writes are checked overwrite, not atomic replacement or a
power-loss guarantee; native failure can leave a partial destination file.

## Protocol, ownership and tests

Viewport, style and modal field values precede snapshot fragments; the final
selection commits one complete view. Each view has a nonreused ticket, including
scroll/zoom without content changes. Stale geometry is rejected. The renderer
uses a bounded 4096-byte UTF-8 window; full source remains in Dis. Long lines can
select a source suffix and follow horizontal pixel geometry. No private pointers
cross heaps. The entry stack roots borrowed channels until acknowledged service
shutdown and explicit detach; both VM heaps/collectors remain separate.

`test_graphical_editor.py`, `test_graphical_actions.py` and
`test_graphical_zoom.py` under `disVM/runtime/` exercise actual compiled modules,
channels/GC, glyphs/raster and files with independent expected results.
`--events FILE` reads bounded little-endian length + `fvd1:` frames; EOF without
acknowledged Quit fails. `--capture FILE.ppm` records real pixels.
`--window-events FILE` additionally checks SDL upload using the same replay;
CTest's dummy driver does not establish physical device behavior. Zoom checks
start with bundled Noto and compare actual raster size changes/equivalence.

Acceptance is experimental Linux AMD64 NC64, one native owner. The SDL pinch
path is not Android-specific and can serve Windows/macOS/Linux touch-capable
frontends, but physical devices and complete other-platform runtime ports remain
separate work. The viewport is fixed 960×640, with logical rather than soft-wrap
navigation. Full bidi/grapheme editing, resize, syntax colors, completion popups,
AS-worker execution, compiler commands and debugger UI remain open. Noto glyph
coverage is finite; missing glyphs do not alter Unicode source/saved bytes.
Synchronous I/O may block the sole native owner.

[Source editing / редактирование](../../EDITING.md).
