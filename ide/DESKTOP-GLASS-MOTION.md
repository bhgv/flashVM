# Glass rails, docking and whole-table reveal

[Русский](DESKTOP-GLASS-MOTION.ru.md) · [Workspace](DESKTOP-WORKSPACE.md) ·
[Dis geometry owner](dis/bridge/idedesktop.b) · [MovieClip adapter](../disVM/runtime/desktop_view.c) ·
[Actual interpreter/JIT tests](../disVM/runtime/test_desktop_glass_motion.py)

Windows can slide beneath either side rail. Their signed table-local coordinates
remain owned by Idedesktop; source documents stay with independent pane processes.
The left rail is80 physical pixels wide and the right rail104. Rails keep their
own input regions even where their backgrounds are transparent.

## Dock and restore

Docking uses the **window's right edge**, not the pointer: when that edge reaches
x100, at most20 pixels remain beyond the left rail and the pane becomes hidden.
This happens during motion, without waiting for release. The pre-drag normal
position/size are saved. The document remains alive and its thumbnail remains a
checked live mirror. A partially covered pane is not automatically docked merely
because the pointer crosses x80.

A thumbnail click or right swipe reveals its pane whether hidden or already open.
Idedesktop computes a viewport target that exposes the complete pane, with its
left edge at least5 pixels beyond the left rail and its right edge5 pixels before
the right rail. Vertical scroll brings the whole pane into view. It changes the
whole table's translation, preserving every window's logical position and relative
spacing. If the pane is larger than the usable viewport, table zoom reduces only
as much as necessary. Show All now includes negative window origins in its bounds.

Movement and uniform zoom interpolate over240ms using `t*t*(3-2*t)`, giving zero
endpoint velocity. Restoring a hidden pane starts at the matching thumbnail;
selecting an exposed pane only pans/zooms the shared table when needed. A reversal
starts from the current visual snapshot. Resize or a direct viewport gesture
cancels an incompatible old trajectory. Retiring an identity clears its motion.

## Owners, clocks and raster workers

Dis decides visibility, focus, positions, thumbnail order and viewport targets.
The native adapter interpolates only copied presentation values. It does not
modify source, schedule a Dis document request per animation sample, or keep a
cross-heap pointer. Source updates do not restart a trajectory.

Native SDL polling advances elapsed monotonic milliseconds independently of SWF
frames and the movie frame clock. It posts the existing frame-free paint
notification and coalesces outstanding work. Keyboard events retain their fast
path. Once240ms is reached no further animation redraw is requested.

The FlashVM owner snapshots genuine MovieClip fills, glyph contours and clipping.
Workers receive immutable native commands, render dirty tiles in parallel and
never enter either VM. Glyph layouts are reused while transforms move. Glass
contours are cached until panel dimensions change; grain is not regenerated per
animation sample. Transient window images retain90-percent opacity.

## Glass appearance

Both panels use60-percent base-opacity tinted fills, a deterministic subtle grain,
and a smooth inward32-pixel fade. The left/right fades are mirrored. Grain varies
tint without stacking alpha, and no opaque backing rectangle covers windows below.
This is an imitation of frosted glass, not a background-blur filter. Window and
widget corner shapes retain the existing rounded MovieClip contours.

## Typed presentation details and repeatable checks

BeginView accepts the original32-byte metadata and extended40/48-byte forms.
The extension carries signed horizontal pan and an explicit Dis reveal epoch;
ordinary scrolling does not become a lagging animated camera. Item X now admits
signed32 values in the bounded negative range, while the hidden65536 sentinel
remains explicit. Native conversion avoids implementation-defined signed casts.
No native address or AS object is a protocol value.

The replay-only request65574 carries0…1000 elapsed milliseconds. The application
acknowledges it without changing document/gesture state; native sessions obtain
elapsed time from SDL. It is a deterministic clock input, not physical touch proof.
Tests check both alpha fades from actual captured pixels, signed under-rail
geometry, full-table left/right/vertical reveal, elapsed-time S curves, docking,
restoration and no restart after completion in both real Dis dispatch modes.

```sh
export TMPDIR="$PWD/build/tmp"
ctest --test-dir build/runtime -R 'desktop_glass_motion|desktop_workspace' --output-on-failure
```

The installed editor package includes these guides and matching Dis modules.
The ordinary build-menu desktop entry remains8; this is the same IDE shell.
