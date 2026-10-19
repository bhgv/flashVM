# Root sound ordinal snapshots — v0.637

[Русский](SOUND_SNAPSHOT.ru.md). This supplements [root sound delivery](MOVIECLIP_SOUND.md).
[`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L66) delivers root StartSound notifications through
[`fvm_movieclip_set_sound_callback`](../include/flashvm/movieclip.h#L21); no audio device is opened by these changes.

A sound-bearing frame's private `sounds` table is a dense zero-based list, bounded
by [`FVM_MOVIECLIP_SOUND_MAX_FRAME_EVENTS`](../src/standard/movieclip_sound_constants.h#L8) (65536). Delivery captures scalar event
indices in ordinal order, independent of hash insertion order. Every selected value
must be an integral, nonnegative Number below the existing canonical byte-derived
ceiling. Holes, wrong types, nonfinite/fractional/out-of-range values and oversized
lists reject capture with [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) before its first host callback.
A truncated dense prefix is structurally valid in this revision; this does not yet
prove canonical frame membership, completeness or uniqueness of the indices.

The complete native vector is prepared before external effects. A malformed later
entry cannot follow an already delivered earlier event. Native snapshot allocation
failure returns [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) without delivering any event from that batch.
Once callbacks start, earlier host effects remain committed on a later error.
Source identity checks and the missing-callback [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) policy retain
priority before this capture. A source-matching empty list with a callback does
not deliver anything. The full-source hex comparison remains linear in file size.

[GC:] The existing outer native tick barrier retains managed metadata and source
values through synchronous delivery; snapshot storage contains only copied native
indices and is released on all exits. Callback registration/source reload/reentry
stay blocked by the existing busy guard, which restores on error and exceptions.
Host callback/userdata are borrowed and must remain alive until unregistration or
context destruction. The callback contract still forbids editing clip/frame data
or executing script. Tests deliberately inject private metadata mutation to verify
robustness; this does not authorize those operations in a host callback.

Root frame entry is marked consumed before sound preparation. Capture, source,
callback and allocation errors stop its later frame actions without rolling back
the visual scene or automatically replaying sounds. A stopped frame stays consumed;
a playing timeline can move to another frame. Explicit host seek/recovery is required
to retry the failed frame. Interpreter and native JIT use the same delivery path.

New regressions: `sound_snapshot_order` (SWF6–8, shuffled insertion, actual native
JIT, defensive scalar/list mutation and committed callback errors),
`sound_snapshot_invalid` (complete local structural/numeric preflight), and
`sound_snapshot_allocation` (allocation sweep, GC/guard restoration, retained
allocation counts). Existing sound delivery/validation/transport/allocation tests
and the [C89 mixer tutorial](../examples/multimedia/swf_event_mixer/README.md) remain
applicable. The tutorial uses only supported public APIs and never edits metadata.

All roadmap stages remain open. No original-Player scheduling, audio hardware,
foreign-platform or full SWF8 compatibility claim follows from these fixtures.

## Delivery work (v0.638)

[`FVM_ExecutionWorkScope`](../src/core/execution_work_scope.hpp#L9) spans source comparison, ordinal capture and delivery.
It inherits an outer AVM1/tick budget and nested cooperative host work rather than
resetting it. One unit is charged before comparing each canonical source byte,
one before copying each ordinal and one before each callback. With N source bytes
and K captured events this is N+2K native units, excluding outer timeline work and
cooperating host queries. A frame with no sound metadata adds zero sound work.

Exhaustion returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) before the next native unit; counters never wrap or
reset when already saturated. Preparation failure yields no sound callbacks;
delivery exhaustion preserves completed callbacks. Source mismatch can fail after
charging the compared prefix. A finite ceiling can now stop source comparison
before the missing-callback check. An instruction limit of zero removes the numeric
ceiling while retaining periodic worker cancellation. Arbitrary host callback wall
time is not preempted; byte comparison and local list work are logical units.
Existing root-entry consumption and explicit seek/recovery policies are unchanged.

`sound_snapshot_budget` checks every exact boundary, nested callback work and
saturated inherited counters. `sound_snapshot_worker` uses two actual OS workers:
one stops at the first source-comparison unit, and another stops during a real
200-event canonical batch after its first callback. These are software notifications,
not audio playback or original-Player scheduling evidence.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `fvm_movieclip_set_sound_callback` | function | [include/flashvm/movieclip.h:21](../include/flashvm/movieclip.h#L21) |
| `FVM_MOVIECLIP_SOUND_MAX_FRAME_EVENTS` | macro / define | [src/standard/movieclip_sound_constants.h:8](../src/standard/movieclip_sound_constants.h#L8) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
