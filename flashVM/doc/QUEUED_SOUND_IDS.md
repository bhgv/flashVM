# Queued sound identities

## v0.793: prepared duration on the output grid

[`fvm_swf_sound_voice_output_frames`](../include/flashvm/swf_sound_mixer.h#L44) counts exact remaining output frames for a
prepared native voice adopted at its CURRENT source cursor with conversion phase
zero. It includes remaining repetitions, rounds up the rational source/output
ratio and returns zero at EOF. It does not read PCM, allocate, call a callback,
access a VM or advance the source. Output rate must be within the configured
mixer range; bad arguments return INVALID_ARGUMENT and preserve output. A
count beyond SOUND_OUTPUT_MAX_CLOCK is UNSUPPORTED. Caller retains and serializes
the voice. The result is an integer-valued double, not seconds or a DAC position.
An already adopted mixer voice has its own lookahead/phase and is not described
by this fresh-grid calculation.

`swf_sound_voice_output_frames` compares every cursor in a three-repeat interval
at four source and eight output rates with actual one-frame mixer retirement,
including EOF and post-VM lifetime. This is native sample accounting, not full
Flash timeline or physical-device acceptance.

## v0.794: optional fixed identity storage

[`fvm_swf_sound_output_set_identity_tracking`](../include/flashvm/swf_sound_output.h#L61) enables/disables a fixed native table
of OUTPUT_ID_SLOTS double deadlines (512 KiB on this host). Configure only with
no mixer voices, accepted or pending PCM; enabling allocates transactionally,
disabling actually frees storage. Invalid state/arguments return INVALID_ARGUMENT;
allocation failure returns OOM and preserves the old table. No backend/VM is
called. All mutations require external output/mixer serialization.

[`fvm_swf_sound_output_storage_info`](../include/flashvm/swf_sound_output.h#L74) copies exact output object, retry PCM,
optional history, optional deadline and checked total byte charges. Borrowed
mixer/device, VM and allocator overhead are excluded. Invalid size/null/reentry
preserves output. [`fvm_swf_sound_output_sound_active`](../include/flashvm/swf_sound_output.h#L81) observes the exclusive FIFO
and compares one ID's deadline with accepted-minus-queued progress. Disabled
tracking returns UNSUPPORTED; invalid ID/reentry returns INVALID_ARGUMENT;
backend errors/exceptions preserve query output. These callbacks may block under
the backend contract. No VM or new allocation is entered. Use tracked starts
and stops exclusively; direct mixer mutation is outside the accounting contract.

`swf_sound_output_identity_storage` verifies optional charges, repeated enable,
real freeing on disable, empty/played-state restrictions and query failures.

## v0.795: queued SyncNoMultiple adoption

[`fvm_swf_sound_output_submit_prepared_bounded`](../include/flashvm/swf_sound_output.h#L114) observes FIFO progress, then adopts
a caller-owned native voice at submitted+pending frames. Its exact remaining
converted duration defines an exclusive per-ID endpoint. Ordinary same-ID starts
may overlap; their maximum endpoint stays active. NoMultiple consumes an unused
owner while that ID has unplayed registered time, even if the software mixer has
already retired the voice. It suppresses without requiring extra mixer budget.
Finished voices do not extend endpoints. Registration never remixes accepted or
pending PCM: the new voice starts at the next composition boundary.

Enable tracking first and route every borrowed-mixer start/read/stop through the
output. The mixer byte budget excludes output/table/device/template storage.
No allocation, decode or VM access occurs; backend observations can block. Keep
all owners serialized/live. Success consumes and clears the handle; failure
preserves its progress and old intervals/voices, although a successful FIFO
observation can refresh copied queue counters. Clock/duration overflow is
UNSUPPORTED; disabled tracking is UNSUPPORTED; mixer/backend statuses propagate.

`swf_sound_output_identity` reproduces the retired-but-queued case, checks the
exact rational endpoint rather than a whole prepared-block boundary, and verifies
failed-write retention/exact retry, no-budget duplicate consumption and failed
queue-observation ownership preservation after VM destruction.

## v0.796: stop and clear ownership

[`fvm_swf_sound_output_stop_id`](../include/flashvm/swf_sound_output.h#L122) applies the borrowed mixer's immediate/DECLICK
future-composition policy and releases that ID's NoMultiple registration. Missing
IDs succeed; invalid IDs/null/reentry return INVALID_ARGUMENT; disabled tracking
returns UNSUPPORTED. It allocates nothing and invokes no callback or VM. Accepted
and pending PCM are immutable; this command does not remove a previously queued
same-ID sample. A new NoMultiple start is allowed after the command, including
while old PCM/release tails remain. Serialize output/mixer externally.

Whole-output stop and stop_release reset all enabled registrations only AFTER
acknowledged backend clear. Failed pause/clear retains registrations, old voices
and pending PCM (an acknowledged pause remains visible). Successful clear cancels
IDs even when a later release write fails. That retained release is anonymous;
new registered voices begin after its exact pending frame count. No retry remixes
it. `swf_sound_output_identity_stop` exercises those failure boundaries, explicit
same-ID restart, missing/invalid IDs and unchanged accepted PCM.

## v0.797: desktop event scheduling

The desktop event output enables fixed identity tracking before worker startup.
Owner-side NoMultiple prechecks and final prepared adoption use that output's
FIFO identity clock, while decode remains outside the worker mutex. Stop-by-ID
uses the output's tracked control; StopSounds retains the acknowledged-clear
policy. Root event-only playback gains this behavior; stream-only production is
unchanged, and root stream-plus-StartSound remains unsupported.

All device/output/mixer operations stay under the existing native mutex. No VM
or managed pointer enters the worker. Identity/output buffers are separate from
EVENT_BYTES source/mixer accounting and are visible through native storage_info.
This fixes premature NoMultiple release at software retirement; it does not add
hardware audibility, sub-frame authored timing or reference Flash A/V clocks.

## v0.798: actual SDL owner-frame regression

`flashvm_frontend_queued_identity` uses a real paused SDL queue to retain a
seven-source-frame voice after its native mixer slot has retired. A second
actual root frame delivers NoMultiple and must not create another voice. Later
frames run stop-by-ID/restart, AVM1 StopSounds and a new NoMultiple start; GC runs
between deliveries. After worker resume the copied dequeue clock expires the ID.
Five real AVM1 frame actions assert interpreter/native-JIT entry counters. The
short-voice queue is primed directly under the producer mutex while the worker
is suspended; no dummy substitute backend supplies the queued observation.
This is Linux/SDL dummy-driver ownership evidence, not physical DAC timing.

## v0.799: C89 ABI and backend reentry

`swf_sound_identity_c89` is compiled as strict C89 and uses an independent raw
SWF with IDs 7 and 65535. Both voices retire in one output block yet remain
NoMultiple-active; independent ID cancellation preserves the other registration.
It checks all new C signatures/storage and exact eight-frame converted duration
after VM teardown. Backend queued callbacks deliberately reenter tracking,
storage, adoption and stop-ID APIs with live caller-owned arguments: each returns
INVALID_ARGUMENT without changing outputs or ownership. External serialization
is still required; reentry rejection is not a cross-thread lock.

## v0.800: allocation rollback and failure ownership

`swf_sound_output_identity_allocation` isolates the optional table's sole native
allocation at allowances zero and one. Failure preserves preexisting output
storage; enable succeeds with one allocation. With all further new/new[] calls
forbidden it adopts native owners, handles pending write rejection/exact retry,
suppresses duplicates, observes IDs, preserves a failed-clear registration,
clears/releases/disables storage and closes native owners after VM teardown.
`swf_sound_output_identity_exceptions` verifies backend bad_alloc/other exceptions
map to OOM/ERROR without consuming an incoming handle or publishing query output;
storage and stop-ID controls do not call that backend. Source/mixer/queue owners
still require explicit external serialization.

## v0.801: annotated C89 FIFO host

The runnable [swf_queued_identity tutorial](../examples/multimedia/swf_queued_identity/README.md)
prepares an original seven-sample SWF, destroys the VM, then demonstrates short
software retirement versus queued identity lifetime on an explicitly modeled
FIFO. It checks the exact eighth-frame endpoint, NoMultiple restart, one failed
write/exact retry, ID cancellation and acknowledged output clear. Optional file
export contains exactly two identical 16-frame accepted stereo blocks; an
independent integer oracle checks every S16 value and CLI/output errors. The
model is caller-owned and deterministic; it does not open or certify a device.
Each native owner/budget and every nonempty C/header line is documented.

## v0.802: copied native deadline diagnostics

[`fvm_swf_sound_output_identity_info`](../include/flashvm/swf_sound_output.h#L99) observes the FIFO and copies requested ID,
played epoch cursor, last registered exclusive end, time until that end and active
state. Exact struct_size and ID 1..65535 are required. Invalid/null/reentry is
INVALID_ARGUMENT, disabled tracking UNSUPPORTED; backend statuses propagate and
exceptions map as for sound_active. Errors preserve caller output. No allocation,
decode, VM or managed pointer is involved; retain/serialize borrowed owners until
callback return. Expired IDs retain a historical end but report inactive/zero
until_end. Cancelled/cleared IDs report end zero. until_end includes any queued
gap before a future start: it is NOT a count of remaining nonzero sound samples
or DAC latency. Boolean activity uses the same validated diagnostic path.

`swf_sound_output_identity_info` checks exact live/expired/restarted/cleared
fields, an intervening queued-silence gap and unchanged outputs after failures.
The C89 reentry fixture and runnable FIFO tutorial also exercise this API.

## Final v0.802 evidence and scope

One output/mixer represents one SWF dictionary identity domain. Equal numeric
IDs from different movies are the same key; a root replacement must clear/rebuild
that owner. The desktop's existing joined root-clear path releases the old table.
Native FIFO progress is not DAC audibility or a reference Flash A/V clock.

On this Linux host, 89 focused native sound/stream/codec and actual C89 checks
passed, together with 15 real SDL dummy-player regressions. Both independent
MP3 stream and three-codec event sample oracles ran, including actual interpreter/
native-JIT actions. Full minimal-core ASan/UBSan passed 15 selected success/status
and C89 checks plus the separately invoked table-allocation-success/no-new-
allocation path. The extended same-ID shorter/longer deadline test passed both
ordinary and full-core ASan/UBSan. LeakSanitizer was disabled because /proc is
unavailable here. Injected allocation/exception unwinding is not claimed under
ASan: both isolated table allocation boundaries and backend exceptions passed
ordinary execution and scoped UBSan of changed voice/output modules, linked to
the ordinary remaining core. No ThreadSanitizer/device/foreign execution is claimed.

Version-only CMake refresh, deterministic source links, sorted catalog and changed
tutorial comment/API checks passed. The global multimedia comment checker still
reports only two byte-identical baseline Windows speech files; it is not claimed
globally passing. All nine roadmap stages stay open. Root stream-plus-event
integration, child streams, full reference seek/A/V, remaining codecs and the
other graphics/text/AS/JIT/GC/debugger work remain separate completion tasks.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_output_identity_info` | function | [include/flashvm/swf_sound_output.h:99](../include/flashvm/swf_sound_output.h#L99) |
| `fvm_swf_sound_output_set_identity_tracking` | function | [include/flashvm/swf_sound_output.h:61](../include/flashvm/swf_sound_output.h#L61) |
| `fvm_swf_sound_output_sound_active` | function | [include/flashvm/swf_sound_output.h:81](../include/flashvm/swf_sound_output.h#L81) |
| `fvm_swf_sound_output_stop_id` | function | [include/flashvm/swf_sound_output.h:122](../include/flashvm/swf_sound_output.h#L122) |
| `fvm_swf_sound_output_storage_info` | function | [include/flashvm/swf_sound_output.h:74](../include/flashvm/swf_sound_output.h#L74) |
| `fvm_swf_sound_output_submit_prepared_bounded` | function | [include/flashvm/swf_sound_output.h:114](../include/flashvm/swf_sound_output.h#L114) |
| `fvm_swf_sound_voice_output_frames` | function | [include/flashvm/swf_sound_mixer.h:44](../include/flashvm/swf_sound_mixer.h#L44) |
<!-- source-links:end -->
