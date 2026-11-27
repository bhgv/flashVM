# Menu 8 desktop on two VMs


Building the paired IDE on Linux64 requires CMake 3.10 or later. Set `CMAKE` to select a different executable.

[Русский](DESKTOP-PAIRED.ru.md) · [Entry points](https://github.com/bhgv/flashVM/blob/master/ide/ENTRYPOINTS.md)

The paired IDE runs Limbo `.dis` modules in DisVM and an ActionScript `.swf`
in FlashVM. The machines exchange data through their standard Styx RPC
channel. On Linux64, build the graphical `disvm-editor` package and select
menu 8 or `--launch desktop`. Its installed `run-desktop-ide.sh` runs the
ordinary DisVM and FlashVM player processes. DisVM starts the companion and
passes it one end of a private socket pair. Both retire when the IDE exits.

```sh
python3 build-menu.py --projects disvm-editor --backend cmake --launch desktop --document "example.b"
sh out/disvm-editor/run-desktop-ide.sh --jit --new "drawing.svg"
```

The launcher uses the selected file's directory as the live workspace. With
no selected file it uses the current directory. The installed modules, font
and SWF live in the read-only `/ide-assets` mount. FlashVM reads the font
across the same StyxRPC connection. `--no-jit` is the launcher default.
For deterministic local probes, use `--headless --capture frame.ppm`.
The legacy embedded renderer remains available through `run-desktop-local.sh`.

## Verified paired path

With the terminal SDK, DisVM also builds `desktop-paired.dis`. Run it with the
module directory as the working directory, a presentation `desktop-view.swf`
in that directory, and the player's endpoint mounted at `/rpc`. The Limbo
launcher uploads the movie through `/rpc/.upload`, writes `start` to
`/rpc/.state` and enters `DesktopIDE.initpaired()` on the same owner. The real
`$Unibus.attach` then carries scene and input frames in both directions on
the existing libstyxrpc session.

For a direct TCP launch, run the first command on the presentation machine.
Run the second in the directory with the built `.dis` modules, `desktop-view.swf`
and `fonts/`. Use the listener's reachable address on both command lines.

```sh
flashvm_styx_player --listen 127.0.0.1 7474 --jit --window
disvm --styx-mount-tcp 127.0.0.1 7474 --at /rpc -- --native-spawn --jit desktop-paired.dis
```

The acceptance test below uses an inherited socket by default. `--tcp`
exercises the two native TCP commands on loopback. See the
[player reference](../flashVM/doc/STYX_RPC_PLAYER.md#start-over-tcp) for the
transport contract.

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

To exercise the installed SWF, font and executables, run the same SDL2 fixture
against the package. This check runs both JIT and interpreter. Add `--tcp` to
use the two stock TCP entry points on loopback.

```sh
SDL_VIDEODRIVER=dummy python3 ide/tests/check_paired_desktop.py \
  --installed --disvm out/disvm-editor --native-events build/paired-sdl-input.so
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
the FlashVM presentation.

The two buttons on the right of the vector pane header turn the magnifier
on or off and reset the camera to the fitted page. In magnifier mode, use
the wheel over the page to zoom around the pointer or drag a rectangle to
fit an area. You can resume drawing at the new scale. Limbo owns the camera
and sends its scale and centre with the vector stream. ActionScript uses
the camera for both presentation and inverse pointer mapping, including
inside an MDI parent. Zoomed paths are rasterized into the fixed page
viewport before presentation. The camera stays out of SVG and document
history.

Use `--vector` with `--native-events` in the command above. The check opens a
filled SVG containing a cubic and enough nodes to span multiple fragments,
draws a rectangle with SDL mouse events, saves it, and compares the file and
pixels in interpreter and JIT modes. `--camera` also exercises wheel zoom,
rectangle fitting, reset and drawing at the same screen position with two
different cameras. The native event fixture supplies input only.

### Local Brush and Pen preview

Start a brush or pen stroke on the displayed SVG page. FlashVM draws the
delivered motion samples on a temporary MovieClip using the displayed
camera and tool style. On release it sends page coordinates and elapsed
pointer times over the same UniBus session. Limbo averages interior brush
samples in 40 ms windows before constructing the SVG contour. The exact
press and release points anchor the two ends. These times come from SDL
mouse or touch events and are independent of frame delivery. A stroke
holds up to 4,096 points, with one
slot reserved for the last point. Each fragment carries the scene ticket
and source revision. The pane owner receives one complete stroke and
commits one history entry. The preview runs on the MovieClip owner.

The SVG Brush works like Merge Drawing with Paint Normal in the current
document plane. A stroke joins connected visible fills of the same color
and cuts away covered paint of a different color. If a cut separates a fill
into islands, each remainder can be selected and moved independently. Moving
the upper stroke later exposes the cut. Undo restores the original geometry
in one step. Dots and straight strokes retain four cubic arcs. Gentle turns
keep cubic arcs along the outside of the nib. Closed paths, sharp turns and
reversals can use one fixed round nib swept along the entire path. Loops
preserve empty interiors, and the same vector operation clips to the page.
Long strokes admit up to 128 simplified centerline knots. Their saved SVG
can take part in a later cut or union within the vector operation's limits.
The vector cut and union preserve
untouched Bézier handles in compound paths with holes and islands. A single
stroke can unite several visible same-color fills in the same operation.
Shared straight edges and matching full or partial Bézier segments use the
fill on each side to decide if the edge survives. Curves can also touch at a
shared separating tangent and remain separately selectable. The bounded
vector operation also handles crossings and repeated subpaths within one
fill under SVG nonzero winding. It preserves the original Béziers in
surviving portions, including loops inside one cubic. Unresolved tangencies
and cases outside the vector operation's limits use the bounded mask.

Escape, focus or another input transition can remove an unreleased
preview. A complete replacement scene also removes it and retires a
released preview when the new document is drawn.

Use `--ink` with the same SDL fixture. It suspends its own Dis process
to observe preview pixels and cancellation, then sends a surviving stroke
across more than one fragment and verifies the saved SVG and final raster.
Use `--merge` to paint across an SVG fill drawn twice in the same path, move
the stroke away, and verify the two remaining islands, the original cubic
handles, the untouched hole, and the visible gap.
Use `--sweep` to check the 40 ms threshold, short jitter, a closed gesture's
cubic corners and empty center, and one Undo and Redo before saving the SVG.

Multiple touch contacts and vector editing overlays still need paired integration.

[Limbo launcher](https://github.com/bhgv/flashVM/blob/master/ide/dis/app/desktop-paired.b) ·
[Desktop controller](https://github.com/bhgv/flashVM/blob/master/ide/dis/app/desktop.b) ·
[ActionScript acceptance](https://github.com/bhgv/flashVM/blob/master/ide/tests/paired-desktop.as)
