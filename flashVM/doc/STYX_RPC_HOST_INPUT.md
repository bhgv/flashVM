# Window input for paired movies

[Русский](STYX_RPC_HOST_INPUT.ru.md) · [Paired player](STYX_RPC_PLAYER.md) · [IDE](../../ide/DESKTOP-PAIRED.md)

A movie uploaded to the paired player with `--window` receives the
`hostInput` object. Use it when a remote application owns the editor or
the other input policy. FlashVM samples SDL2 on its window thread. ActionScript
decides which messages to send over the application's existing connection.

| Operation | Behavior |
| --- | --- |
| `hostInput.enable()` | Returns true. On the next window-owner turn the movie starts polling one FIFO in place of ordinary Key, Mouse and ClipActions delivery. |
| `hostInput.take()` | Copies and removes one event. Returns its kind as a number, or zero when no event is available. |
| `hostInput.get(name)` | Reads the named field of the last successfully dequeued event. Treat it as relevant only after a nonzero return from `take()`. |
| `hostInput.enableResize(minWidth, minHeight, maxWidth, maxHeight)` | Requests native-pixel input and resize events when polling is enabled. Each dimension is a whole pixel in 1–4096. Returns true on acceptance. |
| `hostInput.commitViewport(width, height)` | Publishes a whole-pixel raster extent within the movie's resize bounds. The next render and PPM capture use these dimensions. Returns true on acceptance. |
| `hostInput.publishFrame(revision)` | Publishes a whole revision in 0–4294967295. A positive revision requests one raster for the latest committed scene. Zero selects continuous rendering. Returns true on acceptance. |

| Kind | Event and fields |
| --- | --- |
| 1 | Key. `code` is a Flash virtual-key code and `phase` is 1 for press/repeat or 0 for release. |
| 2 | Pointer. `x` and `y` are logical stage pixels. `code` is 1 for primary or 2 for middle. `phase` is 1 for motion, 2 for press or 3 for release. The first touch contact supplies primary-pointer transitions. |
| 3 | Vertical wheel. `wheel` is signed scroll lines, positive for up. `x` and `y` are logical pixels and `control` is the copied Ctrl state at the event. |
| 4 | Committed text. `text` is a counted Unicode string from SDL text input. |
| 5 | Focus. `phase` is 1 for gained and 0 for lost. |
| 6 | Window close request. An opted-in movie handles it as an application event. |
| 7 | Window resize. `x` is the new width and `y` the new height in native window pixels. |

The FIFO holds 128 events. Consecutive drag moves of the same button can
collapse to the latest point. When full, the poller yields before removing
another event from SDL. Escape reaches the opted-in movie as a key. Focus
loss resets the native modifiers and is delivered in FIFO order. Movie
replacement retires the previous movie's input events. Methods run on the
VM and presentation owner. The snapshot carries copied numbers and strings.

After `enableResize`, pointer coordinates use native window pixels. Consecutive
undelivered resizes can collapse to the latest extent. A movie whose layout is
computed by a peer can hold the preceding frame while it waits for approval,
then call `commitViewport` with the acknowledged dimensions. Invalid or
out-of-range method arguments raise an input error. SDL updates the texture on
its owner turn and retains the same window and input queue.

Choose a new positive revision when a complete scene changes.
The player presents the latest revision and reuses that raster during input
polling. SDL expose and device recovery can repaint it. Window extent changes
also trigger a render, and explicit PPM capture renders the current scene.
The paired IDE uses this with [MovieClip snapshots](MOVIECLIP_SNAPSHOT_BITMAP.md).

For a remote editor, `MovieClip.getChildDepthAtPoint(x, y)` takes world pixels
and returns the frontmost direct visible MovieClip's depth, using bounds.
`MovieClip.getTextScalarAtPoint(depth, x, y)` uses the TextField at that depth
and returns an insertion position in Unicode scalars, or `-1` outside the
field. `TextField.getScalarCount()` uses the same indexing.
`TextField.setSelectionPreview(anchor, caret, highlightRGB, caretRGB)` stores
scalar endpoints and 24-bit colors for the next draw. Equal endpoints draw a
caret. Either endpoint `-1` hides the preview. The raster draws selected glyph
advances behind the text and the caret over it, clipped to the TextField
viewport. Editing policy, focus and selection ownership stay with the caller.
The preview follows glyph clusters when shaping combines adjacent scalars.

[SDL adapter](../frontend/common/sdl_window.c) ·
[Host methods](../src/styxrpc/player_view_input.inc) ·
[Text geometry](../src/standard/text_display.inc)
