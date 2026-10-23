# Mixed queue BASE/TAIL dequeue accounting

[`fvm_swf_sound_mix_queue_storage_required_for_modes`](../include/flashvm/swf_sound_mix_queue.h#L60) predicts requested native owner/PCM memory with optional ID tracking and a fixed BASE/TAIL FIFO bitmap. Both flags are Boolean; invalid geometry/flags return INVALID_ARGUMENT preserving out, checked size overflow OUT_OF_MEMORY. No allocation/callback/VM/GC occurs. Origin storage adds 32768 bytes for the maximum 262144-frame queue; mixer, voice, backend, allocator and VM storage are excluded.

v0.1038: Add exact mode-aware native storage prediction for optional accepted-frame origin accounting without changing legacy predictor meaning.


v0.1039: Add opt-in fixed bitmap origin accounting, validated FIFO retirement and exact category clock observations while preserving legacy callbacks.


v0.1040: Verify retained TAIL classification, stop and failed-clear ownership, invalid queue growth, callback recursion and exception containment.


v0.1041: Exercise wraparound, bounded admission before render and the last exact epoch frames using explicit seeded native metadata.


v0.1042: Verify paired BASE and TAIL origins and publish the actual successful base admission clock without a fallible post-write callback.


v0.1043: Reject mode allocation transactionally and exercise tracked base/tail retries, observations and acknowledged seek reset with no further allocations.


v0.1044: Verify 256 tracked epochs with exact PCM, held-tail handoff, converted stream EOF and coherent category dequeue totals.


v0.1045: Enable accepted origin accounting in root composition and reject combined ID and bitmap budgets before any SDL endpoint is opened.


v0.1046: Add serialized frontend origin diagnostics and verify exact category counts alongside real FWS/CWS CLI PCM in both VM modes.


v0.1047: Check host pause, root Stop event dequeue, retained source phase, base resume, GC/native JIT and acknowledged origin epoch cancellation.


v0.1048: Add a tenth export mode with budgeted origin tracking, exact gated PCM, retained retries and separate accepted versus drained categories.


## Origin mode and epoch ownership

[`fvm_swf_sound_mix_queue_set_origin_tracking`](../include/flashvm/swf_sound_mix_queue.h#L84) enables a fixed bitmap during an
active zero shared clock with no pending retry. Future mixer voices may already
exist; accepted history may not. Disable only after valid observed FIFO drain and
no retry. Repeated mode selection is an allocation-free no-op. Enable compares
its budget to owner/PCM/current-ID-table/bitmap requests; OUT_OF_MEMORY preserves
the old mode. Optional IDs and origins are independent. Both modes must be disabled
before clock begin/end. Acknowledged adapted.clear resets category counters/ring
and pending while retaining allocated modes; failed clear preserves ownership.
ID-enabled clear retains its established voice/ID cancellation policy.

Null/busy/invalid Boolean/state returns INVALID_ARGUMENT. Backend statuses
propagate and callback exceptions become ERROR. Valid observations may refresh
played counters even when later budget/write/control work fails. Allocation occurs
only during explicit mode configuration. Retain and externally serialize queue,
mixer and exclusive backend through calls; no VM or GC pointers are retained.

## Accepted and dequeued categories

| Origin | Frames represented | Meaning of played/dequeued |
| --- | --- | --- |
| BASE | Every accepted adapted.write frame, including zero base and mixed event contributions | Those BASE positions have left the software FIFO |
| TAIL | Every accepted explicit event-only tail frame, including chosen padding or release endpoints | Those TAIL positions have left the software FIFO |

[`fvm_swf_sound_mix_queue_origin_info`](../include/flashvm/swf_sound_mix_queue.h#L91) requires exact-size output and enabled mode.
It performs one validated FIFO observation and copies BASE/TAIL submitted, played
and queued counts. Disabled mode is UNSUPPORTED; invalid/reentry INVALID_ARGUMENT;
backend/clock failures propagate and exceptions ERROR. Errors preserve caller out.
No allocation, render or source advancement occurs. Category sums equal the shared
accepted/dequeued/backlog counts, with exact integer frames through [FVM_SWF_SOUND_OUTPUT_MAX_CLOCK](../include/flashvm/swf_sound_output.h#L14).
Rendered but rejected retry is excluded until acknowledged; its original BASE/TAIL
classification survives gate changes, voice stop and retry through a different
compatible tail variant. PCM values are never used to guess category.

The fixed ring represents up to [FVM_SWF_SOUND_MIX_QUEUE_ORIGIN_FRAMES](../include/flashvm/swf_sound_mix_queue.h#L54) accepted FIFO positions. Tracked
writes observe dequeue before rendering, then return NOT_FOUND on insufficient
ring room without advancing voices or changing pending bytes. Successful writes
record bits/counters without a fallible post-write callback. Invalid growth or
regression is rejected before ledger retirement. Paired stream snapshots carry
the actual latest admission observation even when the backend consumes between
multiple polls. A later backend consumption becomes visible on the next observation.

## Desktop and C89 use

Root desktop composition budgets/enables IDs and origins before native worker
publication. The 32 KiB bitmap is inside the existing FVM_DESKTOP_AUDIO_MIX_BYTES budget; source,
voice, device, VM and allocator memory stay separate. Insufficient combined budget
is rejected during preflight before opening SDL, including when the older ID-only
requirement would fit. `fvm_desktop_audio_origin_info` copies a fresh native snapshot
under the worker mutex; it requires initialized composition and preserves output
on errors. The main thread performs no PCM production in this diagnostic call.

`flashvm_player --stats movie.swf` reports audio_base_submitted,
audio_base_dequeued, audio_tail_submitted and audio_tail_dequeued for composition.
With --finite-audio-tails or --finite-mixed-tails the existing finite policy applies;
otherwise accepted final padding remains TAIL. Root Stop/gates can dequeue TAIL
while source converter phase and BASE counts remain held. After Play, BASE may
resume from that retained grid. StopSounds/seek acknowledged clear starts a fresh
category epoch, matching the existing cancellation policy. Host pause remains
independent. The C89 swf_stream_mix --origins mode exports seventeen exact frames,
with BASE9/TAIL8 before and after explicit file-consumer drain; the tenth-mode
checker uses the independent rational PCM oracle.

These counters classify output operations. They do not reconstruct SWF sample
positions, decoder delay, original media timestamps, physical DAC time or A/V drift.
Automatic A/V correction/seek preservation, child dictionaries/timelines, full
codec/reference latency, backend/platform/device acceptance and all nine stages
remain open. White-box ring/counter tests seed boundaries; they are not long-duration
or huge-PCM device runs. Shared-owner access still requires external serialization.


v0.1049: Specify origin mode budgets, accepted versus pending ownership, exact dequeue categories, desktop diagnostics and independent ten-mode PCM evidence.


[`FVM_SWFSoundMixQueueOriginInfo`](../include/flashvm/swf_sound_mix_queue.h#L64) is an owned-value snapshot: submitted/played fields are exact integer-valued doubles and queued fields are complete native output frames. Retain the queue rather than any borrowed result pointer.


## v0.1050 verification / Проверка

Mixed BASE/TAIL origin clocks v0.1038–v0.1050: an optional fixed 32 KiB bitmap tracks accepted output origins through validated FIFO dequeue. Adapted writes are BASE (including zero PCM and event mixing); explicit event-only pumps are TAIL (including chosen padding/releases). Pending retries are excluded until acknowledgement and preserve origin across gate/stop/retry changes. Additive storage/mode/observation APIs preserve old predictor meanings and untracked callbacks. Validated writes observe room before render, with no allocation or fallible callback after acceptance. Paired stream snapshots carry the actual latest base admission observation. Desktop mixed acquisition preflights/enables the origin journal inside its existing adapter budget; --stats reports separate accepted/dequeued categories. Validation passes 211 native regression/tutorial/CLI checks, 65 selected ASan/UBSan checks plus thirteen explicit --no-throw runs, and eight SDK-free core/tutorial checks. Additional mixed compatibility and options/help checks pass. Ordinary builds cover exceptions/allocation failures; LeakSanitizer is disabled. Evidence includes seeded wrap/capacity/2^53 boundaries (not huge PCM or duration runs), allocation rollback and zero-allocation scheduling, exact converted PCM and 256 epochs on four independent OS workers. A real SDL dummy worker verifies host pause, root Stop TAIL drain, retained source grid, BASE resume, GC/native JIT and acknowledged epoch cancellation. FWS/CWS CLI tests assert PCM and origin totals in both VM modes. The C89 tenth --origins mode checks 17 exact frames, BASE9/TAIL8 and five retained retries with an independent rational oracle. A renamed detached component configures with explicit SDK and its copied ten-mode checker passes against the native binary; no separate detached build. Origin bookkeeping does not reconstruct SWF source/DAC timestamps or implement automatic A/V/drift/seek correction. Child timelines/dictionaries, complete codec/reference latency, other backend/platform/device acceptance and all nine stages remain open.


v0.1050: Record bounded category ownership, coherent paired clocks, desktop statistics and independent PCM evidence with explicit remaining synchronization limits.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_SWF_SOUND_MIX_QUEUE_ORIGIN_FRAMES` | macro / define | [include/flashvm/swf_sound_mix_queue.h:54](../include/flashvm/swf_sound_mix_queue.h#L54) |
| `fvm_swf_sound_mix_queue_origin_info` | function | [include/flashvm/swf_sound_mix_queue.h:91](../include/flashvm/swf_sound_mix_queue.h#L91) |
| `fvm_swf_sound_mix_queue_set_origin_tracking` | function | [include/flashvm/swf_sound_mix_queue.h:84](../include/flashvm/swf_sound_mix_queue.h#L84) |
| `fvm_swf_sound_mix_queue_storage_required_for_modes` | function | [include/flashvm/swf_sound_mix_queue.h:60](../include/flashvm/swf_sound_mix_queue.h#L60) |
| `FVM_SWF_SOUND_OUTPUT_MAX_CLOCK` | macro / define | [include/flashvm/swf_sound_output.h:14](../include/flashvm/swf_sound_output.h#L14) |
| `FVM_SWFSoundMixQueueOriginInfo` | struct | [include/flashvm/swf_sound_mix_queue.h:64](../include/flashvm/swf_sound_mix_queue.h#L64) |
<!-- source-links:end -->
