# Native clone, remove and drag Actions — v0.366

[Русское описание](JIT_SCENE.ru.md)

CloneSprite, RemoveSprite, StartDrag and EndDrag lower to native helpers using
shared scene services. CloneSprite keeps source/name/depth rooted while the
service converts depth, name and source in that order, applies the absolute-depth
bias and constructs an intrinsic clone. Inputs are consumed only on success.
RemoveSprite resolves/removes its input, then repairs a detached active target to
the surviving base clip or null, matching the interpreter's activation adapter.

StartDrag resolves its target first, then converts center and constrained flags
as Int32 values: only integer one enables them. Optional bounds are converted in
yMax/xMax/yMin/xMin order. The resolved clip has its own temporary root across
collecting/throwing conversions, including conversions that detach it. The shared
begin service publishes the context-owned drag edge without moving the clip.
EndDrag flushes pending motion and propagates errors through native unwinding.
Scene method overrides are bypassed; limits and unsupported native scene cases
remain those of the existing services.

| Test | Coverage |
| --- | --- |
| `jit_scene` | Isolated host/SWF4–8 contexts in interpreter/manual/automatic modes; clone geometry versus custom fields, absolute/public depth, direct clip removal including SWF4, protected negative depths, selected-target repair, bounded/free drag flags and positions, deferred updates and EndDrag flush, absent operands and preserved lower stack values |
| `jit_scene_callbacks` | Exact three-step clone and seven-step drag coercion order, GC/exception at every step, detached unpinned clip retained across later drag conversions and collection, EndDrag update exception and successful retry |
| `jit_scene_allocation` | Host/loaded allocation failures for all four Actions in fresh contexts, restored roots/depths/active code/update state, successful clone/removal/drag/flush and collection after code release |

The baseline clone uses public depth three because the display fixture's source
already occupies depth two. A clone at the same depth correctly replaces its
source; this is distinct from a missing drag target.

This extends native execution of existing scene semantics. It does not complete
Flash duplicate/remove/drag compatibility, imported SWF resource/display-list
behavior, other Actions or real foreign-platform acceptance. No roadmap stage is
closed.

Linux Release: all 747 AGG, 747 Cairo and 728 minimal checks have passing evidence
after generated ELF permission repairs and reruns. Three scene checks pass
restricted heap/global ASan+UBSan; stack ASan and LeakSanitizer are disabled.
