# Player presentation recovery

[Русский](PLAYER_RECOVERY.ru.md). The optional SDL player keeps one owned packed
RGBA copy of the last successfully uploaded frame. The source surface is borrowed
only during `fvm_desktop_window_present`; it may be reused or freed immediately
on return. Row padding is excluded from the retained copy. No VM value or GC root
is stored in the window adapter. Closing the window frees the snapshot before
SDL shutdown. Allocation failures are reported, rather than retaining caller data.

The snapshot has the fixed logical stage size (four bytes per pixel), adding up
to 64 MiB at the existing 4096×4096 player limit. This is in addition to the host
raster surface and SDL texture. It is native presentation storage, not a SWF cache.
A rejected extent or insufficient input byte count leaves the retained image
unchanged. A later accepted presentation replaces it.

`flashvm_frontend_retained_frame` uses an actual SDL dummy software renderer and
checks padded rows, caller-buffer reuse, rejected surfaces and replacement.
This is Linux host protocol/renderer validation; it does not certify visible
windows or Android/Windows/macOS device recovery.

## Device and render-target reset (v0.648)

SDL_RENDER_DEVICE_RESET recreates the streaming texture, sets its blend mode,
uploads the retained frame and redraws it on the main thread. SDL_RENDER_TARGETS_RESET
only redraws: the player uses a streaming texture, not a target texture. A reset
before the first frame creates empty storage without presenting undefined pixels.
No timeline advance, script/timer callback or additional VM rendering occurs.
Exposure/resize also redraw the uploaded texture.

`fvm_desktop_window_status` reports persistent presentation/input failures
separately from the normal close result of `fvm_desktop_window_events`. The player
checks it in active, background and final-frame waits; failed restoration is not
reported as successful user cancellation. Repaint errors are latched as well.
The SDL test injects both reset events and reads actual restored texture pixels
through the software renderer before presentation. It does not reset a GPU.

## Background input isolation (v0.649)

Both WILLENTERBACKGROUND and DIDENTERBACKGROUND clear held keys/buttons, primary
touch and the pending transition FIFO. Key/mouse/finger events while backgrounded
are discarded, including stale Escape/Back. WILLENTERFOREGROUND does not resume;
DIDENTERFOREGROUND does. Quit, owned-window close and graphics reset still work
while suspended. Desktop focus loss retains its previous clear-input policy and
does not pause playback.

Window, keyboard and mouse events carrying a nonzero foreign window ID are
ignored, including foreign Escape and close. ID zero remains accepted for
explicit synthetic/unassigned events. Touch keeps its selected-contact routing;
this change does not claim a complete multi-window SDL host. The lifecycle test
uses the real SDL queue for application events and injected native transitions,
not a physical Android Activity.
