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

Scroll the right toolbar to reach Files, Edit, Search and frequent Undo/Redo/Find.
Files opens a palette to the left, over the desktop: New Limbo, AS1/AS2/AS3, SVG,
Open, Save, Save As, Undo/Redo and four recent paths. Edit duplicates Undo/Redo and
session Copy/Cut/Paste. Search provides literal, case-sensitive Unicode Find and
Replace All. Save and Find are standalone shortcuts too. Ctrl+F/Ctrl+H open search
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
