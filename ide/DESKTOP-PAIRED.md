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
The renderer stages five layers. The table layer uses the Dis pan, scroll and
zoom anchor, so drawing and hit admission share the same coordinate system.
Grouped editors are real children of their MDI MovieClip. Their tabs use the
Dis layout and select the corresponding document owner. Ctrl+G groups an
editor, a tab click switches documents, and Ctrl+D detaches the current editor.
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
An SDL resize proposes the window's width and height using the current scene
ticket. DisVM recomputes and returns the layout. FlashVM applies the new raster
extent after the matching `EndView` and keeps the same native window and queue.
The IDE accepts widths of 240–2048 and heights of 160–2048 pixels.

The left shelf and overview map show small rasters of the staged document
MovieClips, including their text. Docked panes and inactive MDI tabs keep a
local source for the shelf. The miniature and the full document appear with the
same completed scene. The viewport outline leaves the overview previews visible.
Click or drag the left cards to reveal or reorder documents. Click or drag the
overview map to move the viewport. DisVM owns all camera, ordering and focus
changes. FlashVM presents the new raster on each scene revision.

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

Add `--mdi` to that command to resize the SDL window from 960 × 640 to 860 × 620,
edit two documents by switching tabs, detach and rejoin one editor, and verify
both saved byte sequences plus the final active tab and editor raster.

Use `--thumbnails` with `--native-events` to dock a grouped pair, reopen both
documents through the shelf, save independent edits, navigate with the overview
and reorder a card. The final frame checks source ink inside both cards, including
the inactive tab's moved preview, and the overview image and viewport outline.

## SVG documents on the pair

Use Ctrl+Shift+N to create a vector pane. Ctrl+O opens SVG in that pane.
Choose a drawing tool with its usual key, for example R for Rectangle, and drag
on the white page. Ctrl+S saves through the active Limbo document. Ctrl+G places
a vector document in its own graphics MDI group.

The geometry travels on the same UniBus session as input and scene descriptors.
Dis declares the snapshot ticket, pane, document revision and byte count,
then sends consecutive fragments before `EndView`. ActionScript decodes
one record at a time and constructs paths on the staged MovieClip. The new
scene becomes visible after every declared stream completes. Cubic SVG paths
retain their original controls in Dis and use a segmented approximation in
the FlashVM presentation. The paired canvas fits the page to the pane.

Use `--vector` with `--native-events` in the command above. The check opens a
filled SVG containing a cubic and enough nodes to span multiple fragments,
draws a rectangle with SDL mouse events, saves it, and compares the file and
pixels in interpreter and JIT modes. The native event fixture supplies input only.

Multiple touch contacts, the vector viewport and editing overlays, and
FlashVM-local ink preview still need paired integration. Menu 8 currently
starts the installed graphical runtime.

[Limbo launcher](https://github.com/bhgv/flashVM/blob/master/ide/dis/app/desktop-paired.b) ·
[Desktop controller](https://github.com/bhgv/flashVM/blob/master/ide/dis/app/desktop.b) ·
[ActionScript acceptance](https://github.com/bhgv/flashVM/blob/master/ide/tests/paired-desktop.as)
