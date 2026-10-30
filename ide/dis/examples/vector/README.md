# Portable vector model example and starter SVG

[Русский](README.ru.md) · [Full editor guide](../../../VECTOR-EDITOR.md)

The actual editor is inside **menu8 → Draw** (or Ctrl+Shift+N), not a separate
native drawing demo. Max/N makes the active vector window larger. B/R/E/L select
brush/rectangle/ellipse/line,1..6 change colour, F toggles figure fill, +/- changes
width. Ctrl+S/O saves/reads SVG; Ctrl+Z/Y undo/redo a whole gesture.

## Open the supplied drawing

```sh
sh out/disvm-editor/run-desktop-ide.sh --document "$PWD/ide/dis/examples/vector/starter.svg"
```

In the installed package use its `examples/vector/starter.svg` instead. The `.svg`
suffix selects a real vector owner on startup. Its editor window and thumbnail
render the same actual MovieClip paths.

`starter.svg` now includes an XML1.0 UTF-8 declaration and explanatory comments.
The v0.1697 reader accepts them as non-rendering metadata and omits them during
canonical Save. Here is the complete explanation of its drawing source:

- `svg` declares the standard namespace and a1000×600 logical viewBox, matching
  the model. Width/height do not depend on the current desktop/window scale.
- The blue `path` is unfilled, has stroke width12, starts at(100,120), then uses
  two explicit quadratic commands with one control/destination each. The final
  line preserves a straight release segment. These are generic authoring paths,
  not animation instructions.
- The red `rect` has opposite corners(120,340)/(380,500). It is fill-only: no
  stroke attribute means no native outline. Import turns it into a closed path.
- The green `ellipse` has centre(650,420), radii140/90 and width12. Import turns
  it into the model's eight quadratic arcs. Save writes those ordinary paths.

## Compile and run the fully commented Limbo example

[main.b](main.b) comments every code line. It uses the same Idevector model as
MDI panes, commits a continuous blue curve and a filled red ellipse, writes a
real SVG with exclusive creation, reads it back and compares canonical output.
No UI, foreign emu, external SVG library or host Python is needed at runtime.

Build products include `vector-demo.dis`. From a fresh working directory:

```sh
package=/absolute/path/to/out/disvm-editor
mkdir vector-example-work
cd vector-example-work
"$package/disvm" --no-jit "$package/vector-demo.dis"
```

Expected: `PASS vector example: vector-example.svg, two paths, exact SVG roundtrip`.
Open that file with the actual IDE or a normal SVG viewer. The example refuses to
overwrite an existing file; use another fresh working directory for another run.
`--jit` selects real guarded JIT dispatch instead of the interpreter.

To rebuild from the installed source and matched headers:

```sh
cd /absolute/path/to/out/disvm-editor
./limbo-native -I include/limbo -o vector-demo-local.dis examples/vector/main.b
```

Run this module by absolute path from a fresh working directory as above.
The native bootstrap is temporary build tooling; the example itself runs as
portable Dis bytecode. This is not full semantic compilation by our own Limbo→FIR
frontend. Do not mix modules/headers from older packages after the pane interface
extension.

## Ownership and limits

The example owns its document and immutable path/history arrays. MDI uses one
separate Limbo controller per vector pane and only typed-channel presentation.
The SVG subset/resource limits and incomplete I/O caveats are listed in the
[guide](../../../VECTOR-EDITOR.md). It does not silently load unsupported SVG,
perform boolean paint union, implement node selection or animate the drawing.
