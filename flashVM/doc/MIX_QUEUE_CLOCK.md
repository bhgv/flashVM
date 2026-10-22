# Shared composition FIFO clock

v0.939 adds an opt-in clock to the native stream/event composition adapter. [fvm_swf_sound_mix_queue_clock_begin](../include/flashvm/swf_sound_mix_queue.h#L69) requires a serialized live adapter, no pending retry and an empty real FIFO. It observes the backend and enables a fresh zero epoch only after success. Construction stays callback-free and accounting remains disabled until explicitly begun. Restarting an empty drained epoch is allowed. No pause, clear, converter seek or voice reset is inferred.

[fvm_swf_sound_mix_queue_clock_info](../include/flashvm/swf_sound_mix_queue.h#L86) fills an exact-size [FVM_SWFSoundMixQueueClock](../include/flashvm/swf_sound_mix_queue.h#L52): submitted_frames counts every real accepted adapted.write and pump_tail block, played_frames is submitted minus fresh validated queued_frames, and seconds divides this dequeue count by the fixed output rate. Rejected writes count nothing until their exact retained retry succeeds. The maximum exact count is [FVM_SWF_SOUND_OUTPUT_MAX_CLOCK](../include/flashvm/swf_sound_output.h#L14); overflow is UNSUPPORTED before rendering/writing. This is not a DAC timestamp, source EOF, frame authorization or full A/V synchronization.

Impossible queue growth beyond submitted/max backlog or regression of played position returns ERROR without publishing output or changing the last valid played position. Before begin, info is NOT_FOUND without callbacks. Invalid output size/null/busy is INVALID_ARGUMENT with output preserved. Real backend statuses propagate, thrown callbacks become ERROR. Acknowledged adapted.clear resets the active epoch; failed clear preserves counters/pending, and pause preserves them. All calls require external serialization and live borrowed mixer/backend; hot paths allocate nothing and hold no VM/GC references. Public existing structures remain unchanged; native owner storage queries include the added private fields.

The basic fixture proves opt-in behavior, exact rejection/retry counting, queue dequeue and explicit restart. Production desktop integration, device/platform acceptance and all nine roadmap stages remain open.

v0.940: clock_end explicitly disables accounting after producer quiescence, no pending retry and validated FIFO drain. Inactive/nonempty returns NOT_FOUND, pending/null/busy INVALID_ARGUMENT, backend errors propagate. A valid nonempty poll may advance the last played observation while leaving accounting active. No device pause/clear, voice/source reset or ownership transfer is inferred. Tests verify rejected pending/end, unplayed/end, drained/end, legacy count forwarding after end and a fresh explicit begin.

v0.941: callbacks attempt recursive begin/end/info during queue observation, write, pause and clear; every attempt rejects before state publication. Failed begin remains inactive; failed writes/clear/pause and thrown write/queued callbacks preserve retry/accounting. Impossible growth and played regression leave caller snapshots and the last valid position intact. Acknowledged clear resets the active epoch. The ordinary test covers exceptions; --no-throw supports separate sanitizer selection.

v0.942: an explicit white-box seed tests MAX_CLOCK-2, a rejected two-frame block, exact successful retry at MAX_CLOCK and base/tail overflow rejection without voice advance or backend write. After acknowledged clear a real tail starts at the next source sample and counts from zero. The test compiles the actual adapter implementation and seeds only counters; it does not claim real-duration playback of 2^53 frames.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_clock_begin` | function | [include/flashvm/swf_sound_mix_queue.h:69](../include/flashvm/swf_sound_mix_queue.h#L69) |
| `fvm_swf_sound_mix_queue_clock_info` | function | [include/flashvm/swf_sound_mix_queue.h:86](../include/flashvm/swf_sound_mix_queue.h#L86) |
| `FVM_SWF_SOUND_OUTPUT_MAX_CLOCK` | macro / define | [include/flashvm/swf_sound_output.h:14](../include/flashvm/swf_sound_output.h#L14) |
| `FVM_SWFSoundMixQueueClock` | struct | [include/flashvm/swf_sound_mix_queue.h:52](../include/flashvm/swf_sound_mix_queue.h#L52) |
<!-- source-links:end -->
