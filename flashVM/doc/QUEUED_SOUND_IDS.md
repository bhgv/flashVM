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

[`fvm_swf_sound_output_submit_prepared_bounded`](../include/flashvm/swf_sound_output.h#L96) observes FIFO progress, then adopts
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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_output_set_identity_tracking` | function | [include/flashvm/swf_sound_output.h:61](../include/flashvm/swf_sound_output.h#L61) |
| `fvm_swf_sound_output_sound_active` | function | [include/flashvm/swf_sound_output.h:81](../include/flashvm/swf_sound_output.h#L81) |
| `fvm_swf_sound_output_storage_info` | function | [include/flashvm/swf_sound_output.h:74](../include/flashvm/swf_sound_output.h#L74) |
| `fvm_swf_sound_output_submit_prepared_bounded` | function | [include/flashvm/swf_sound_output.h:96](../include/flashvm/swf_sound_output.h#L96) |
| `fvm_swf_sound_voice_output_frames` | function | [include/flashvm/swf_sound_mixer.h:44](../include/flashvm/swf_sound_mixer.h#L44) |
<!-- source-links:end -->
