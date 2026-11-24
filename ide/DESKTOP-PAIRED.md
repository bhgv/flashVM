# Menu 8 desktop on two VMs

[Русский](DESKTOP-PAIRED.ru.md) · [Entry points](https://github.com/bhgv/flashVM/blob/master/ide/ENTRYPOINTS.md)

The ordinary DisVM build now produces the same `desktop-ide.dis` that menu 8
uses. Its pane owners, layout, dialogs and file operations come from the
existing Limbo sources. The graphical menu chooses the installed
`run-desktop-ide.sh`, which executes `disvm-editor` to render the full desktop.

## Verified paired path

With the terminal SDK, DisVM also builds `desktop-paired.dis`. Run it with the
module directory as the working directory, a presentation `desktop-view.swf`
in that directory, and the player's endpoint mounted at `/rpc`. The Limbo
launcher uploads the movie through `/rpc/.upload`, writes `start` to
`/rpc/.state` and enters `DesktopIDE.init()` on the same owner. The real
`$Unibus.attach` then carries scene and input frames in both directions on
the existing libstyxrpc session.

The AS acceptance movie reads the scene with two editors, a build console
and Help. It requests a ticket-bound resize from 960 to 860 pixels, so
the Limbo layout switches to one column. It inserts `Яpaired` into the first editor, waits for the next
view to contain it, drives the Save As dialog and enters `paired-output.txt`.
It waits for the saved document's caption and requests ordinary Quit.
The check requires the original desktop's acknowledged shutdown and checks the
exact bytes on disk from a fresh working directory. It runs with both
interpreter and JIT on the SDK-enabled executables.

From the repository root, configure `disVM/runtime` with the source-matched
`DVM_TERMINAL_LAUNCH_SDK`. Build the ordinary runtime and run the check.

```sh
cmake --build build/disvm-runtime --target disvm disvm_desktop_paired
python3 ide/tests/check_paired_desktop.py --disvm build/disvm-runtime \
  --player build/flashvm/flashvm_styx_player --as2 build/as2/view/as2_viewc \
  --fir build/fir --swl build/linker/swl
```

The check compiles the SWF with AS2 → FIR → SWO → SWL, copies the Limbo-built
modules into isolated working directories, and provides the two VMs with the
ends of one connected socket. Application behavior runs in the compiled Dis
modules and the SWF.

## Paired graphical desktop

Build the production preview with the same tools.

```sh
python3 ide/build_paired_view.py build/disvm-runtime/desktop-view.swf \
  --as2 build/as2/view/as2_viewc --fir build/fir --swl build/linker/swl
```

This movie handles `BeginView`, `ItemView` and `EndView`. It prepares MovieClips
at an inactive depth and publishes them only when the end record matches the
pending ticket and the declared item count. It paints the stage, rails and
rectangular panes from their coordinates and colours the active header.
The table's pan, scroll and scale come from the scene records.
Standard TextFields draw each pane's title and body, along with the labels on
the workspace controls. The same scene that carries positions carries the text.
The visible editor's caret and selection come from the same completed scene.
FlashVM maps primary-pointer hits to Unicode scalar positions using that
field's shaped layout. DisVM validates the scene ticket and owns the edit.

Build the player with `flashVM/cmake/profiles/text-jit.defs` and the SDL2 frontend
to see the text in a window. Place `ide/assets/fonts/NotoSansMono-Regular.ttf`
at `fonts/NotoSansMono-Regular.ttf` in the DisVM launch directory. FlashVM reads
it from the live DisVM VFS over the paired connection at movie setup. The preview
shows panel geometry when the player has no font support or the file is absent.

Run the player with `--window` for keyboard input, committed Unicode text,
mouse selection and vertical scrolling. Ctrl+wheel sends the existing zoom
gesture. Focus loss clears held modifiers in Dis. The native window close
button sends the normal Quit request, including the unsaved-document check.
FlashVM polls its input FIFO through `hostInput` and the SWF sends events through
the same UniBus connection that supplies scenes.

Add `--text --window` to the
repository check to serve the font and verify glyphs in the editor and console
after the resize and edit. `--capture` alone checks the pane, header and rail
geometry. The check links a separate ActionScript observer to the
same production drawing code. The production SWF has no scripted test inputs.
The prebuilt archive contains `desktop-view.swf` for live input and
`desktop-check.swf` for the automated sequence, with the font and its license.
To repeat the sequence, use
the check movie as `desktop-view.swf` in a separate copy of the directory.

On Linux the native acceptance fixture posts SDL2 key, focus, text and pointer
events into the live player. It runs the production movie with SDL's dummy
video driver, edits and saves `Я🙂paired`, and verifies the final selection,
caret and Ctrl+wheel zoom in both dispatch modes.

```sh
cc -shared -fPIC $(pkg-config --cflags sdl2) ide/tests/paired-sdl-input.c -ldl -o build/paired-sdl-input.so
SDL_VIDEODRIVER=dummy python3 ide/tests/check_paired_desktop.py \
  --disvm build/disvm-runtime --player build/flashvm/flashvm_styx_player \
  --as2 build/as2/view/as2_viewc --fir build/fir --swl build/linker/swl \
  --native-events build/paired-sdl-input.so
```

Native window resize still scales the authored stage. Resizing the Dis layout
from that window event, multiple touch contacts, MDI composition, live thumbnails
and vector bulk/ink still need integration. Menu 8 continues to use the installed
graphical runtime.

[Limbo launcher](https://github.com/bhgv/flashVM/blob/master/ide/dis/app/desktop-paired.b) ·
[Desktop controller](https://github.com/bhgv/flashVM/blob/master/ide/dis/app/desktop.b) ·
[ActionScript acceptance](https://github.com/bhgv/flashVM/blob/master/ide/tests/paired-desktop.as)
