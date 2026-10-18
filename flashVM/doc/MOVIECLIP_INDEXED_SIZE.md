# Indexed MovieClip dimensions — v0.379

ActionGetProperty indices 8 (width) and 9 (height), including the shared C
execution service, now read the current vector geometry. Empty clips return
zero. The queried clip's local matrix and descendant local matrices transform
the paths before their bounds are combined. Translation cancels in the extent;
reflection, rotation, authored shear and descendant offsets contribute. Ancestor
matrices are excluded, so even a singular ancestor does not prevent a read.
The existing stroke inflation rule is included.

Named `_width`/`_height` replacements do not intercept these indexed reads.
Index coercion still precedes target resolution and can run script/GC. The
selected input/target stays rooted even when a coercion detaches it. Successful
results may alias an input. Errors and allocation failure leave output intact.
Since v0.385 indexed writes are supported in the bounded vector model
(MOVIECLIP_DIMENSION_WRITES.md). No public ABI, callback ID or
graph format changes.

[GC:] Geometry traversal holds the native collection barrier and copies each
node's path/child slots before replaceable transform getters run. Reentrant
removal therefore cannot invalidate an iterator or collect a detached object in
the snapshot. The current node's captured children participate in that read;
the next read observes removal. Later nodes capture their own state when reached.
This is the existing native geometry model, not a complete Flash intrinsic
matrix/callback contract.

Remaining bounds: named dimension accessors/setters, exact Flash twip rounding,
bitmap/text/filter/mask/scrollRect extents, precise stroke joins/caps and full
resource/display-list conformance. The existing double path/stroke model and
bounded geometry recursion remain. Extremely large coordinates can lose extent
precision. No original player oracle or foreign-OS run is claimed.

Primary implementation comparison:
[pinned Ruffle width/height and local_bounds](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/display_object.rs).
This supports the parent-space choice; it does not certify all geometry details.

Tests: movieclip_indexed_size covers host/SWF4–8 empty/transformed/descendant
bounds, shear, singular ancestors, aliasing and a write smoke check (updated in v0.385).
movieclip_indexed_size_callbacks covers removal during a transform getter,
named getter bypass, errors, actual collecting index coercion, detachment,
exceptions and root/barrier restoration. movieclip_indexed_size_allocation
sweeps both read allocation boundaries and verifies unchanged output, GC and
successful retry. jit_indexed_size compares interpreter, explicit and automatic
native execution for host/SWF4–8, including missing targets.

All nine roadmap stages remain open.

Validation v0.379: all 785 AGG / 785 Cairo / 766 minimal Linux Release checks
have passing evidence after nine executable-permission repairs on eight ELF
files and targeted reruns (four repair operations AGG, three Cairo, two minimal).
Eight restricted heap/global ASan+UBSan checks pass: the four new dimension
checks plus standard MovieClip, AVM1/JIT display properties and geometry
allocation. Stack ASan and LeakSanitizer remain disabled. No new external SWF
oracle or foreign-OS runtime acceptance is claimed. All nine stages remain open.
