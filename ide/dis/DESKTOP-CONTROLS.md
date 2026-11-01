# Desktop controls and source ownership

The desktop is one MovieClip scene. MDI is a window with tabs on that desktop;
AS, Limbo and vector documents have separate tool/navigation domains. SDI detaches
a tab without replacing its document owner. Grouping and closing use the typed
membership service; presentation parents do not acquire source authority.

| Module | Responsibility |
| --- | --- |
| `bridge/idetabview` | Tab/body geometry and tab hit admission |
| `bridge/idesourcedomain` | Case-insensitive source-extension classification |
| `bridge/idedocking` | Dock when the dragging contact enters the left shelf |
| `bridge/idepick` | Reverse-paint-order scene hit testing |
| `bridge/idethumbnails` | Portrait card spacing and stable reordered identities |
| `bridge/iderails` | Independent left/right scroll ranges and physical rail regions |
| `bridge/idetools` | Toolbar labels, palette rows and command mapping |
| `bridge/idetoolpalette` | Shared button grid, owner anchor and popup hit geometry |
| `bridge/iderecent` | Four session-only recent paths and sanitized display labels |
| `bridge/idedesktop` | Layout/gesture owner; composes the controls above |
| `app/desktop` | Document/channel registry and command coordination |
| `../as/chrome/*.as` | Independently compiled background, rails, cards, headers, buttons and fields |

## Input and commands

Drag empty desktop space, use three contacts, middle mouse, or Ctrl+Shift+left
drag to pan the desktop. The same pan admission over either rail scrolls that
rail instead. Ordinary empty rail margins/gaps drag vertically; wheel events are
routed by pointer position. One-contact thumbnail dragging reorders cards;
clicking or swiping a card right reveals its document. Rail offsets are independent
of document geometry and desktop zoom/pan. Portrait cards keep their readable
size as more documents open; overflow is reached by scrolling.

The right shelf is 60 pixels wide. Its 44×27 buttons sit 6 pixels from the outer
edge, with a 32-pixel vertical step; a short viewport still scrolls independently.
Files, Edit, Search and View open compact rounded bubbles to their left. A triangular
tip points to the invoking button. Bubble buttons use exactly the same dimensions
and compiled AS appearance as shelf buttons; gaps and recent-label text are inert.
The grid uses three columns, or four on short canvases. Files has New Limbo,
AS1/AS2/AS3, SVG, Open, Save, Save As, Undo/Redo, four recent files, Close and Quit.
The short R1–R4 captions refer to the numbered path legend below the buttons;
the full paths remain solely with the application owner. The legend is omitted
below 300 pixels of canvas height. Edit has Undo/Redo and session Copy/Cut/Paste;
Search has Find and Replace (`Repl.`). View has Show All, Max/Norm, MDI, SDI and Close.
Palette-only commands have no duplicate shelf buttons. The frequent Save, Undo,
Redo and Find shortcuts intentionally remain on the shelf. Build, Debug and Help
are direct buttons; Help toggles its owned pane.

Ctrl+F/Ctrl+H open search
fields. Find selects the next match and wraps; Replace asks for a needle, then a
replacement, applying one undoable core transaction on the exact current revision.
These are literal operations, not regular-expression or cross-project search.

New typed source documents remain untitled; a proposed suffix is offered at Save.
Their MDI domain is retained before a real path exists. Save As explicitly prompts
even for a named document. Recent paths live only in the current application session;
Open still checks unsaved edits. The renderer receives shortened labels, not authority
to open paths. The session clipboard remains distinct from the operating-system one.

## Presentation execution

AS chrome is compiled with the real Coco/R AS view frontend through FIR, object
and SWF linking, then executed through FlashVM's AVM1 interface. The current view
profile supports scalar assignments/arithmetic; it is not a claim of full AS
function/control-flow lowering. `ideDraw` is a rooted native capability exposing
generic drawing operations through ordinary properties. Geometry/theme values
come from AS, layout/commands from Limbo. Native C supplies input, graphics capture,
immutable raster jobs and compositing. No VM object is used on raster workers.

The reference appearance uses grey paper cards, blue headers, dark right buttons
and translucent graduated rails. The glass imitation uses layered alpha strips;
it does not implement an actual background-blur shader. Bitmap/tile reuse and
asynchronous rasterization remain separate from legacy timeline frame callbacks.
The private scene has96 slots for controls; this is not a Dis64 file-format ID change.

See [C module example](../../flashVM/examples/embedding/native_module/README.md),
[existing extension examples](../../flashVM/examples/extending/README.md) and
[Dis native modules](../../disVM/runtime/NATIVE_MODULES.md).

## Reused MovieClip backgrounds

Hiding an absent scene slot clears its graphics but retains its MovieClip for
reuse. Clearing now invalidates the chrome cache at the same ownership boundary.
The next appearance rebuilds its AS graphics even when dimensions/selection match
the previous appearance. Position-only changes retain the normal cache benefit.
Popup role and preview scale also participate in the chrome key; a failed repaint
cannot leave an apparently valid old key. No global cache disable or frame delay
is introduced.

`disVM/runtime/test_desktop_pool.py` compares actual before/reopen RGB captures,
including after docking and panning and for every command bubble, in interpreter
and JIT dispatch. The first modal comparison fails against the previous binary.
`test_desktop_rails.py` exercises physical button hits, common extents, right
alignment, real source creation/history/search/clipboard and independent scrolling.
The Limbo desktop test covers the 320-pixel viewport and a scrolled popup anchor.

Maximized windows and MDI containers now use the unscaled physical overlay, keeping
fonts and controls at 1:1 independently of the saved desktop/window zoom. Restoring
keeps the camera and normal geometry. See [native ink](../NATIVE-INK.md) for the C
trajectory buffer, stamp raster and separate ordered Limbo geometry worker.
