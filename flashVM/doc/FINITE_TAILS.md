# Finite mixed event tails

[`fvm_swf_sound_mixer_active_frames`](../include/flashvm/swf_sound_mixer.h#L54) queries a bounded remaining output horizon on the current mixer grid, including fractional lookahead, repeats and release. It does not consume PCM or infer physical audibility. All owners require external serialization; no allocation, callback, VM or GC occurs. maximum is 1..[FVM_SWF_SOUND_MIXER_MAX_HORIZON](../include/flashvm/swf_sound_mixer.h#L46); null/invalid input returns INVALID_ARGUMENT preserving out. Empty mixer returns OK with zero. The result is clamped to maximum and silent-valued source samples still count.

v0.1012: Add allocation-free active frame horizons including fractional lookahead, repeats and release without consuming PCM.


v0.1013: Check independent durations and PCM across four source clocks, six output rates, channel mappings, repetitions and partial source adoption.


v0.1014: Add opt-in horizon-limited tail pumping while preserving indivisible retries from every earlier rendering mode.


v0.1015: Expose opt-in finite frame-gate and EOF scheduling with coherent shared clocks and unchanged base and retry ownership.


v0.1016: Check strict and converted release endpoints, earlier source exhaustion and bounded huge-repeat arithmetic without huge allocations.


v0.1017: Add explicit finite mixed-tail startup policy and CLI option before worker publication while preserving default playback behavior.


v0.1018: Check exact short WAV output under root Stop and future gates for FWS/CWS in interpreter and JIT, plus device-free mute.


v0.1019: Exercise callback errors and recursion, unchanged pending bytes after stops, failed clear preservation and zero native allocations after preparation.


v0.1020: Verify 256 epochs with rejected held tails, newly permitted base, exact saturated PCM, source EOF and queued identity expiry.


v0.1021: Add a ninth mode with four exact rejected blocks, a short final tail, unchanged base and explicit queued identity drain.


v0.1022: Extend the rational PCM checker to all nine modes and assert exact finite output, four rejected blocks and drained sound identity.


## Finite native producers

[`fvm_swf_sound_mix_queue_pump_tail_finite`](../include/flashvm/swf_sound_mix_queue.h#L270) uses the adaptive FIFO contract of
[`fvm_swf_sound_mix_queue_pump_tail_up_to`](../include/flashvm/swf_sound_mix_queue.h#L261), additionally limiting fresh zero-base
work to the live mixer horizon. maximum is 1..queue capacity; target is positive
and bounded by SOUND_OUTPUT_MAX_QUEUE. accepted is optional and cleared first.
No voices/full FIFO/insufficient retry room returns NOT_FOUND; null/busy/invalid
geometry or a pending BASE returns INVALID_ARGUMENT. Shared-clock overflow is
UNSUPPORTED before rendering, callback errors propagate and exceptions are ERROR.
A retained TAIL is indivisible, even when it was prepared by a padded API or voices
were stopped after rejection. Retry its original size and bytes; acknowledged
clear is the cancellation boundary. New silent-valued source/release frames still
count. This trims synthetic block padding, not authored silence or release endpoints.

[`fvm_swf_stream_output_pump_mix_finite_until`](../include/flashvm/swf_stream_output.h#L114) adds the same policy to the existing
paired stream scheduler. It validates the exact adapted backend and common epoch,
accepts a retained TAIL before newly permitted base, holds BASE when its frame gate
is closed and preserves source phase during event-only output. Base conversion is
unchanged. Frame zero holds base while permitting voices. Optional accepted clears
first; OK can also consume a zero-frame marker. NOT_FOUND is not source EOF: it can
mean missing clock, headroom, held BASE or exhaustion. Other native/backend statuses
propagate. Both counters use the successful write admission observation, without
a fallible callback after acceptance. Retain and externally serialize stream,
converter, snapshot, queue, mixer and exclusive backend; no allocation, VM or GC.
Legacy paired/exact/adaptive functions keep their prior behavior and ABI.

## Desktop use

Run `flashvm_player --finite-mixed-tails movie.swf` to select finite root mixed
event tails. This flag affects only stream-plus-event composition: event-only and
stream-only paths retain their existing policies. With `--mute` no device is opened.
`fvm_desktop_audio_open_mixed_policy` validates both Boolean flags and initializes
policy before publishing the native worker; old open functions select padded tails.
The policy stays immutable while the worker runs. Completed-frame publication,
authored root Stop, host pause, StopSounds, seek and common queued identities retain
their existing contracts. Short finite queues are explicitly resumed to drain even
when they cannot reach the normal driver reserve.

The C89 `swf_stream_mix --finite` tutorial exports fourteen exact event-only frames
in 4,4,4,2 blocks after source VM teardown, deliberately rejecting all four blocks.
Its ninth-mode independent rational oracle checks PCM, counts and ID expiry. Native
evidence covers conversion/repeats/channel/partial-adoption matrices, strict and
converted release, bounded huge-repeat arithmetic, controls/reentry/exception and
zero-allocation paths, and 256 epochs on four independent OS workers. Actual C89
desktop scheduling asserts interpreter/native JIT entries with GC; CLI FWS/CWS
Stop/gate/mute tests use a real SDL dummy worker and exact 9/14-frame WAV oracles.
The huge-repeat test seeds bounded metadata; it is not a huge allocation/device run.

This does not add child dictionary/timeline routing, reference codec latency/seek,
physical DAC timestamps, A/V drift correction or other backend/platform/device
acceptance. All nine roadmap stages remain open.


v0.1023: Specify live horizon bounds, indivisible retries, source and release limits, explicit player activation and remaining reference and device acceptance.


## v0.1024 verification / Проверка

Finite mixed tails v0.1012–v0.1024: a bounded live-grid mixer horizon includes fractional lookahead, remaining repeats and the shorter source/release interval. New finite queue and paired stream APIs trim only fresh event-only block padding; all existing retries remain indivisible, including padded retries created by older APIs. Desktop --finite-mixed-tails selects immutable startup policy before worker publication; default, event-only and stream-only paths retain their prior behavior. Root Stop/gates, host pause, StopSounds/seek and queued identities preserve ownership. Validation passes 191 native regression/tutorial/CLI checks, 51 selected ASan/UBSan checks plus eleven explicit --no-throw runs, ten SDK-free core/tutorial checks and two additional options/help checks. Four existing white-box queue limits were rebuilt and rerun; no huge source/device run is inferred from seeded arithmetic tests. Ordinary builds cover exceptions/allocation failures; LeakSanitizer is disabled. Native evidence includes an independent conversion/duration/PCM matrix, strict/converted release endpoints, zero-allocation final retry/cancel and 256 epochs on four independent OS workers. Real C89 worker tests assert interpreter/native JIT entries and GC; FWS/CWS Stop/gate CLI checks exact 9/14-frame WAV and mute on SDL dummy. The ninth C89 export mode checks 4,4,4,2-frame tails with an independent rational PCM oracle. A renamed detached component configures with an explicit SDK and its copied nine-mode checker passes against the native executable; no separate detached build. Child dictionaries/timelines, full codec/reference latency/seek, physical DAC/A-V/drift, other backend/platform/device acceptance and all nine stages remain open.


v0.1024: Verify explicit policy parsing and Boolean validation, and record finite mixer, player and tutorial contracts with remaining acceptance limits.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pump_tail_finite` | function | [include/flashvm/swf_sound_mix_queue.h:270](../include/flashvm/swf_sound_mix_queue.h#L270) |
| `fvm_swf_sound_mix_queue_pump_tail_up_to` | function | [include/flashvm/swf_sound_mix_queue.h:261](../include/flashvm/swf_sound_mix_queue.h#L261) |
| `fvm_swf_sound_mixer_active_frames` | function | [include/flashvm/swf_sound_mixer.h:54](../include/flashvm/swf_sound_mixer.h#L54) |
| `FVM_SWF_SOUND_MIXER_MAX_HORIZON` | macro / define | [include/flashvm/swf_sound_mixer.h:46](../include/flashvm/swf_sound_mixer.h#L46) |
| `fvm_swf_stream_output_pump_mix_finite_until` | function | [include/flashvm/swf_stream_output.h:114](../include/flashvm/swf_stream_output.h#L114) |
<!-- source-links:end -->
