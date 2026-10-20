# Player RGBA frames for C hosts

[Русский](FRAME_CAPTURE.ru.md) · [Control](PLAYBACK_CONTROL.md) ·
[Controller threads](PLAYBACK_MAILBOX.md) · [Roadmap](ROADMAP_STATUS.md).

## Contract and copying since v0.716

[FVM_DesktopFrame](../frontend/common/frame.h#L18) describes a borrowed owner-thread native raster observation:
RGBA8, width/height, stride, byte_count, pixels/source and tick/presentation counts.
[FVM_DESKTOP_FRAME_RGBA8](../frontend/common/frame.h#L5) means straight-alpha R,G,B,A bytes, left to right and rows
top to bottom, not an endian-dependent integer. Each visible row contains
width * [FVM_DESKTOP_FRAME_PIXEL_BYTES](../frontend/common/frame.h#L7) bytes. Dimensions are bounded by
[FVM_DESKTOP_FRAME_MAX_DIMENSION](../frontend/common/frame.h#L8), the current stage/raster extent limit.

It is the logical scene, not a window screenshot: window chrome, DPI/letterboxing
and native resize swaps are excluded. Reasons are [FVM_DESKTOP_FRAME_TICK](../frontend/common/frame.h#L10) after
a host advance and [FVM_DESKTOP_FRAME_REFRESH](../frontend/common/frame.h#L11) for initial/dirty redraw without
another host tick. Presentation counts start at one; movie_ticks/presentations
belong to the current root and total_* belong to the complete run.

[FVM_DesktopFrameNotify](../frontend/common/frame.h#L44)(frame, userdata)` runs on the owner after successful raster
and native presentation, outside VM rendering. [FVM_OK](../include/flashvm/flashvm.h#L42) continues; any other result
fails playback after drawing/VM effects have already committed. Borrowed frame/
pixels/source are valid only until return. Do not modify pixels, retain pointers,
reenter VM/player, shut down SDK, steal SDL events or throw across C. Copy pixels
into independent storage before handing them to another thread. [GC:] No managed
owner/pin is supplied: retaining frame does not retain its native buffer or VM.

[FVM_DesktopFrameObserver](../frontend/common/frame.h#L47) is a separate descriptor; old source/options/loader/
control layouts remain unchanged. [fvm_desktop_frame_observer_init](../frontend/common/frame.h#L57)(out)` fills
exact struct_size and null notify/userdata, returning 1/0 for nonnull/null with
no allocation/SDK startup. An observed runner requires notify. The descriptor and
userdata stay live until synchronous runner return.

[fvm_desktop_frame_copy](../frontend/common/frame.h#L65)(frame, destination, capacity, destination_stride)` copies
visible RGBA row bytes only, without allocation, VM or SDL startup. On [FVM_OK](../include/flashvm/flashvm.h#L42) the
source is unchanged and destination padding stays untouched. Errors return
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) without modifying any destination bytes. Validation covers
pointers, exact struct_size, format, nonzero bounded dimensions, both strides and
both accessible extents, including overflow in (height-1)*stride+row_bytes.
Source final-row padding is not required: the complete visible last row suffices.
Metadata/source and padding are not copied. For one row an enormous stride alone
is not invalid if the visible row is accessible.

Destination must be accessible and not overlap source: this is a caller ownership
precondition, not arbitrary C-pointer provenance detection. Do not pass freed or
inaccessible storage. Every geometry/capacity check finishes before copying begins.

## Verification and limits

`frontend/tests/frame_copy.c` uses an independent raw-byte oracle: unequal strides,
preserved padding, truncated source/destination, unknown format, overflow and
untouched error output. The C89 test starts neither VM nor SDL. v0.716 introduces
the contract/copy utility; runner integration is documented as it is added. This
is not a windowless widget, new GPU control or completion of roadmap stages.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
