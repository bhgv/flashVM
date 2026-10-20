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
