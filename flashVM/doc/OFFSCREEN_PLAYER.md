# Windowless logical-stage playback

[Русский](OFFSCREEN_PLAYER.ru.md) · [Frame API](FRAME_CAPTURE.md) · [Roadmap](ROADMAP_STATUS.md).

The timer-only presentation adapter owns an SDL timer reference and validates the
same raster geometry as native presentation. It creates no window/renderer/texture,
does not initialize video/events, never polls or removes the embedding application's
SDL events and never queries or changes its cursor. Close releases only its own
reference. Logical stage dimensions remain bounded by the software rasterizer.

This is software RGBA output, not GPU capture or an operating-system widget.
SDL remains a build/link dependency for timing, threading and optional audio;
no device or foreign native-OS acceptance follows from this adapter alone.
