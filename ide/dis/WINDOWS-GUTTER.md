# Responsive window buttons and hideable line numbers

[Русский](WINDOWS-GUTTER.ru.md) · [Graphical editor](examples/graphical-editor/README.md) ·
[Live projections](LIVE-PROJECTIONS.md)

## Window lifecycle

The SDL adapter now recognizes both application SDL_QUIT and the main window's
SDL_WINDOWEVENT_CLOSE. Previously the per-window close notification was ignored.
RESIZED and SIZE_CHANGED both produce a checked size proposal; MAXIMIZED and
RESTORED query the current SDL window extent and use the same path. Foreign
window events do not affect the editor. EXPOSED repaints the last acknowledged
view without editing the document. Canvas updates avoid requesting an SDL size
that the window already has, preventing needless resize feedback.

These events become copied requests, never direct document mutations. Resize
still passes through Dis revision/ticket/row-capacity checks and the existing
240..2048 by 160..2048 allocation bounds. The renderer, caret and mouse layout
are updated together. Minimize remains an OS window operation; restored/exposed
notifications repair presentation when the window is shown again.

## Unsaved close is visible

A clean close releases the projection adapter, stops the document and detaches
native channel borrowing. A dirty close opens a read-only confirmation instead
of silently returning Conflict:

- S saves and closes only after successful output and MarkSaved acknowledgement.
- D explicitly discards unsaved text and closes.
- Esc cancels and returns to editing.

A document without a path enters Save As after S. Failed output keeps the window
open. Cancelling Save As cancels the pending close; a later unrelated save cannot
unexpectedly close the window. Closing again does not imply permission to discard.
The pre-existing explicit force-Quit/Shift-close contract remains available.

## Line numbers

The left gutter is visible by default. Ctrl+L hides/shows it without a source
edit, saved-state change or Undo entry. `--line-numbers 0` starts hidden;
`--line-numbers 1` starts visible. The native host returns this initial preference
by value; subsequent changes are owned by the Limbo controller and sent as
Gutter view envelopes (65563), tied to the document revision after Style.

Numbers are one-based logical LF lines, including the final empty line. A scrolled
viewport continues from its Dis-supplied top line. Width follows visible digit
count and actual font advances. Numeric glyphs use the same font/size and actual
source-layout baselines. The field's width, source ink, caret/selection, horizontal
reveal and mouse coordinates all account for the strip. Hiding it reclaims the
space. Clicking a number aligns the source caret with that visual row; it does
not insert/delete source or run a command. No second source parser was added.

Native code formats only presentation values, never computes source history or
changes visibility directly from keyboard input. Default Noto and size 11 remain
unchanged. Older native hosts/controllers must be rebuilt together because the
Idehost interface now includes the initial preference getter and Gutter frames.

## Checks and build

The SDL window test pushes actual SDL events through a real dummy-driver window
and the production translator. The Dis integration tests check close prompt,
cancel, discard and actual save/clean shutdown. Gutter tests inspect real glyph
pixels, source/caret geometry, mouse hits, scrolling and hide/show in interpreter
and AMD64 JIT. SDL upload/clipboard/resize tests use the existing dummy driver.
These checks do not claim physical desktop/touch acceptance.

```sh
# Obtain a system or pinned local SDL SDK without installing system packages.
python3 tools/sdl2/ensure_sdk.py --workspace . --build-root build --cmake cmake
# Supply the SDK config directory printed by that command.
cmake -S disVM/runtime -B build/runtime -DDVM_RUNTIME_EDITOR=ON \
  -DDVM_EDITOR_SDL=ON -DSDL2_DIR=/absolute/path/to/lib/cmake/SDL2
cmake --build build/runtime
ctest --test-dir build/runtime -R 'sdl_window|graphical_(gutter|close)' --output-on-failure
```

A console/headless build can still omit SDL. The line gutter itself needs only
FlashVM's existing CPU font/vector renderer. Network RPC, arbitrary debugger
locals and exported-object GC remain separate from this window/editor work.
