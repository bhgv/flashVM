# Independent display and event hosts

Since v0.598, `flashvm/host.h` provides a C89 boundary without SDL, Clutter,
Cogl, GLES or window-system types. The VM continues to render into the
host-owned [`FVM_RasterSurface`](../include/flashvm/raster.h#L16). Hosts choose their own event loop, GPU upload,
texture lifetime, scheduling and keyboard/contact policy. The SDL player is
one consumer of this contract, not a mandatory VM dependency.

## Frame delivery

`fvm_host_present(surface, byte_count, callback, userdata)` checks non-NULL
pixels/callback, nonzero dimensions, packed RGBA8 row width, stride, address-size
arithmetic and available bytes before entering the backend. `byte_count` is the
actual accessible allocation span from `surface->pixels`, supplied by the owner;
the function cannot discover whether an arbitrary pointer is valid. Padding
between rows is accepted; final-row trailing padding is not required.
[`FVM_HOST_RGBA_BYTES`](../include/flashvm/host.h#L8) is four. Channel order is RGBA, alpha is straight,
coordinates start at the top-left, and stride is in bytes. Conversion to
premultiplied alpha, BGRA or another row orientation belongs to the backend.

The [`FVM_HostPresentCallback`](../include/flashvm/host.h#L11) receives a copied descriptor and borrowed pixels,
synchronously on the caller's thread. It must copy/upload the bytes before
returning, or make its own owned copy for asynchronous submission. The caller
keeps descriptor/storage alive and stable during the call. No buffer ownership
is transferred and the API retains nothing. `userdata` is passed unchanged,
including NULL. GPU/window resources remain host-owned.

Validation failure returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) and invokes no callback.
Otherwise the callback is called exactly once and its status is returned;
[`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) is not converted into success. A C++ `std::bad_alloc` is
contained as [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44), other C++ exceptions as [`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Hardware
faults, longjmp and dangling buffers are outside this contract. There is no VM
context or GC activity; obey the backend's main-thread/graphics-context rules.
The SDL adapter additionally rejects frame dimensions different from its
texture and strides exceeding the SDL signed pitch range before upload.

## Event coordinates

`fvm_host_pointer_to_world(world_to_host, host_x, host_y, world_x, world_y)`
inverts the supplied [`FVM_Affine`](../include/flashvm/path.h#L33) into world pixels. Pass the transform from
SWF world coordinates to the coordinate space of the actual event. Compose
stage origin, actor transform, letterboxing and device scale exactly once.
For example a scale of two with translation (100,50) maps event (120,90) to
world (10,20). Rotation, shear and reflection are supported. Negative/outside
coordinates remain valid: this function does not clip or decide hit testing.

All arguments and coefficients must be finite; the matrix must be nonsingular.
NULL or identical output pointers, unrepresentable arithmetic and nonfinite
results return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), preserving both outputs. Linear
coefficients are normalized before determinant evaluation to handle large
uniform scales; numerically collapsed determinants are rejected. This is host
double arithmetic, not SWF binary32 matrix emulation. No allocation, VM call,
callback, event dispatch or GC occurs, so independent storage may be used on
independent threads. Do not mutate the same arguments concurrently.

Feed successful results to [`fvm_input_dispatch_mouse`](../include/flashvm/mouse.h#L25) on the VM owner thread;
use [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L79) for complete key snapshots. Hosts own first
contact selection, cancellation, synthesized-event suppression, focus reset
and event ordering. Do not enter the VM from a platform input worker: copy
native events and drain them on its owner thread. SDL already normalizes
window/touch coordinates into its logical stage, so another window-scale
conversion there would apply the transform twice. This change does not replace
its mixed FIFO or add automatic Flash focus selection.

## Future Clutter/Cogl host

Inspected shell revision
[2a616b264f0045a3ba00ce77aa1930ca905956ea](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/tree/2a616b264f0045a3ba00ce77aa1930ca905956ea),
only outside `inferno-os`. In
[app/jni/clutter_main.c](https://github.com/bhgv/Inferno-OS_Android-Clutter3d/blob/2a616b264f0045a3ba00ce77aa1930ca905956ea/app/jni/clutter_main.c),
the shell uploads RGBA rows with `clutter_texture_set_from_rgb_data` and receives
Clutter touch events via `clutter_event_get_coords`. A separate host can consume
FlashVM frames through that texture boundary and map stage/actor event
coordinates using the inverse contract above. Its GLES/context and actor
lifecycle stay in the shell. Do not wire FlashVM into the existing `xmouse_btn`
client path: that path belongs to a different client integration.

This is a verified generic host boundary and an integration design, **not an
implemented Clutter backend or Android device acceptance**. No files/dependencies
from that repository are copied, and `inferno-os` is excluded. Future adapter
code belongs in `ports/`, with an explicit external SDK and native lifecycle,
input cancellation and pixel-format acceptance tests. Direct GLES rendering,
zero-copy GPU textures, full multitouch and platform keyboard/IME mapping
remain separate work. Roadmap stages 1–9 remain open.

## Validation

`flashvm_tests_host` tests padded/final-row buffers, short spans, overflow,
callback failure/exception containment, singular/nonfinite transforms, rotation,
shear and large scales. `flashvm_frontend_window` tests the actual SDL
consumer's bounds rejection and byte-correct pixels. The frontend is C89 and
the implementation is C++98. See the current stage report for executed runs.
## Executable host tutorial (v0.599)

[Headless C89 host](../examples/embedding/host_backend/README.md) renders a
real raster square, validates/consumes its borrowed RGBA frame, converts a
scaled/translated event, and dispatches down/up into a context without SDL.
The SDL FIFO now uses [fvm_host_pointer_to_world](../include/flashvm/host.h#L26) for each saved event as well;
SDL has already normalized its coordinates, so its remaining matrix restores
only the authored stage origin. Event batching/error/focus-reset rules stay
the same. The example needs a raster backend and explicitly skips without one.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Affine` | struct | [include/flashvm/path.h:33](../include/flashvm/path.h#L33) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_host_pointer_to_world` | function | [include/flashvm/host.h:26](../include/flashvm/host.h#L26) |
| `fvm_host_present` | function | [include/flashvm/host.h:18](../include/flashvm/host.h#L18) |
| `FVM_HOST_RGBA_BYTES` | macro / define | [include/flashvm/host.h:8](../include/flashvm/host.h#L8) |
| `FVM_HostPresentCallback` | callback typedef | [include/flashvm/host.h:11](../include/flashvm/host.h#L11) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:79](../include/flashvm/keyboard.h#L79) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:25](../include/flashvm/mouse.h#L25) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_RasterSurface` | struct | [include/flashvm/raster.h:16](../include/flashvm/raster.h#L16) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
