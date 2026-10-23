# Finite event-only audio output

The opt-in [`fvm_swf_sound_output_pump_finite`](../include/flashvm/swf_sound_output.h#L133) stops fresh PCM at the live mixer horizon, preserving every pending retry, including legacy or stop_release bytes. It does not remove silent-valued source frames or release endpoints. accepted clears first; empty future voices return NOT_FOUND without silent priming. Same owner lifetime, serialized backend callbacks, clock bounds and exception/status contract as the original pump; no allocation, VM or GC. The original pump retains continuous silence behavior. NOT_FOUND is not queue drain or physical audibility.

v0.1025: Add opt-in finite output pumping with exact final frames and unchanged legacy, release and pending retry ownership.


v0.1026: Add a serialized pending observation with coherent dequeue clock updates and preserved caller output on errors.


v0.1027: Verify failed clear and write ownership, exception containment, callback reentry and anonymous release completion after voices retire.


v0.1028: Check NoMultiple through dequeue, the last exact clock frame and zero allocations across short rejected retries and cancellation.


v0.1029: Verify 256 epochs with exact short retries, independent rational PCM, coherent pending observations and queued identity expiry.


v0.1030: Add immutable event policy, explicit finite-audio-tails CLI parsing and compatibility wrappers for previous mixed-only startup.


v0.1031: Hold event-only production during source preparation and frame callbacks, then authorize native work from the owner safe point.


v0.1032: Pump finite event blocks, resume short queues, observe voices and retries through final FIFO drain and avoid expected-idle underrun reports.


v0.1033: Check simultaneous opposite voices, short and multi-quantum tails, FWS/CWS, native JIT entries, exact PCM and device-free mute.


v0.1034: Exercise host pause, completed frame publication, root Stop, native JIT and GC, exact dequeue completion and post-stop anonymous release.


v0.1035: Extend the queue tutorial with explicit finite exhaustion, retained legacy PCM, pending FIFO observation and acknowledged software drain.


## Pending work and native controls

[`fvm_swf_sound_output_pending`](../include/flashvm/swf_sound_output.h#L142) queries live future voices, retained retry and fresh
accepted FIFO under one output callback barrier. It refreshes the validated dequeue
clock but does not render, seek or consume source PCM. Required live serialized
output/out; paused owners still report work. Invalid/reentry is INVALID_ARGUMENT;
backend statuses propagate, impossible/regressing queue observations are ERROR,
bad_alloc is OUT_OF_MEMORY and other exceptions ERROR. Errors preserve out; a valid
observation may refresh internal played time. No allocation, VM or GC occurs.
Zero means no current work, not a reservation against a later voice submission and
not a DAC timestamp. NOT_FOUND from pump alone does not establish FIFO drain.

Finite pump uses the same fixed buffers and native budget as ordinary pump.
It fits fresh work to FIFO headroom and live mixer horizon. Clock overflow is ERROR
before rendering, as in ordinary pump. Retained PCM from either pump or stop_release
is accepted whole; failed clear preserves it, acknowledged stop cancels the epoch.
Queued NoMultiple remains active through finite pending/accepted time after voice
retirement. A new start after an idle gap starts on the existing accepted-frame
epoch; wall-time silence is not synthesized or added to these queue clocks.

## Desktop finite completion

`flashvm_player --finite-audio-tails movie.swf` enables finite event-only and root
stream/event tails. Existing `--finite-mixed-tails` stays mixed-only; both defaults
remain unchanged. Stream-only playback keeps its existing policy, and --mute opens
no device. `fvm_desktop_audio_open_policy` validates three Boolean startup flags
and publishes immutable policy before creating the worker. Previous acquisition
functions retain their established behavior.

For finite event-only mode, each owner callback withholds production before source
preparation outside the mutex. `fvm_desktop_audio_timeline` publishes readiness only
after complete frame delivery, so same-frame voices share one composition grid.
Embedding hosts must perform this publication after delivery even under authored
root Stop. Host pause still holds the entire output. Previously accepted FIFO can
drain while future production is withheld; it cannot be selectively recalled.

The worker pumps finite blocks, resumes a short queue that cannot reach the normal
driver reserve and treats expected end/idle as completion rather than an underrun.
The existing serialized `fvm_desktop_audio_stream_pending` also observes finite
event-only work, including pending release with no mixer voices. Suspended hosts
report no processable completion work; this is not cancellation or a drain claim.
The player's bounded tail loop waits for current voices/retry/FIFO, while retaining
host STOP/pause/navigation/deadline behavior. Legacy event-only completion is unchanged.
StopSounds keeps its explicit acknowledged clear and anonymous boundary release.

## Evidence and boundaries

Native tests cover exact rational PCM, short retry, NoMultiple dequeue lifetime,
seeded exact-clock boundaries, zero allocations and 256 epochs on four independent
OS workers. Real SDL dummy tests cover withheld frame publication, host pause,
authored root Stop, GC/native JIT entries, exact drain and anonymous stop release.
CLI FWS/CWS interpreter/JIT tests verify simultaneous opposite voices and complete
short/multi-quantum event PCM, including output longer than the queue target.
The C89 swf_audio_queue --finite mode isolates empty voices, legacy retry and
acknowledged software drain; it is intentionally a silence protocol demonstration.
The separate swf_stream_mix nine-mode PCM oracle remains unchanged.

This does not implement Sound class API, child dictionary/timeline routing,
reference codec latency/seek, physical DAC or A/V timestamps/drift, or other
platform/device acceptance. All nine roadmap stages remain open.


v0.1036: Specify native pending and release semantics, explicit player activation, owner publication and mixed-policy compatibility with remaining acceptance limits.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_output_pending` | function | [include/flashvm/swf_sound_output.h:142](../include/flashvm/swf_sound_output.h#L142) |
| `fvm_swf_sound_output_pump_finite` | function | [include/flashvm/swf_sound_output.h:133](../include/flashvm/swf_sound_output.h#L133) |
<!-- source-links:end -->
