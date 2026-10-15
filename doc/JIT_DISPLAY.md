# Native display properties and paths — v0.362

[Русское описание](JIT_DISPLAY.ru.md)

GetProperty, SetProperty and TargetPath lower to direct native helpers using the
shared scene/property/path services. Display target, index and assigned value
remain on the rooted operand vector through index conversion, path resolution,
value conversion and callbacks. Reads replace the first input before popping the
index; writes consume their inputs only on success. TargetPath keeps its source
rooted through boxing and native counted-path publication.

Existing semantics and limits remain shared with interpretation: property indices
are normalized before target text; invalid targets, invalid indices and read-only
fields do not coerce an assigned value. Unsupported writable fields keep their
service status. Selection follows the active timeline target independently from
script-this. TargetPath uses the native dot path, bypassing script toString and
_target overrides, and preserves embedded NUL in the native name. Nonclips return
undefined after the existing boxing policy.

| Test | Coverage |
| --- | --- |
| `jit_display` | Isolated host/SWF4–8 contexts in three modes; x/y writes and reads, direct/path/selected targets, absent targets/operands, native path bypassing script overrides, counted-NUL names, nonclips and retained path after code release/GC |
| `jit_display_callbacks` | Exact index/target/value conversion order; collecting/throwing conversions; read-only/invalid-index/absent-target writes skip value coercion |
| `jit_display_allocation` | Host/loaded failures across collecting conversion, property mutation/read and path publication; restored roots/depths/active code; retained output after release/collection |

The fixture accesses a direct MovieClip through a member of an ordinary object:
SWF4 GetVariable deliberately hides real MovieClip results, while GetMember does
not. The helper now accepts a receiver so prepared scene tests use their real
base timeline; existing class tests retain the default receiver.

These additions do not implement all display fields or all Flash scene semantics.
Other Actions, complete reference-player compatibility and real foreign-OS/CPU
acceptance remain open; no roadmap stage is closed.

Linux Release: all 736 AGG, 736 Cairo and 717 minimal checks pass after restoring
generated ELF execute permissions. Six display/class checks pass restricted
heap/global ASan+UBSan; stack ASan and LeakSanitizer remain disabled.
