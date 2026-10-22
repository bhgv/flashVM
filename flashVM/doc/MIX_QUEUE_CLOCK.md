# Shared composition FIFO clock

When optional [queued identities](MIX_QUEUE_IDENTITIES.md) are enabled, clock_begin/clock_end reject; disable tracking first at a drained safe point. Acknowledged adapted.clear resets the shared epoch and cancels all tracked voices/deadlines. This explicit opt-in policy does not change legacy untracked clear.

v0.939 adds an opt-in clock to the native stream/event composition adapter. [fvm_swf_sound_mix_queue_clock_begin](../include/flashvm/swf_sound_mix_queue.h#L69) requires a serialized live adapter, no pending retry and an empty real FIFO. It observes the backend and enables a fresh zero epoch only after success. Construction stays callback-free and accounting remains disabled until explicitly begun. Restarting an empty drained epoch is allowed. No pause, clear, converter seek or voice reset is inferred.

[fvm_swf_sound_mix_queue_clock_info](../include/flashvm/swf_sound_mix_queue.h#L86) fills an exact-size [FVM_SWFSoundMixQueueClock](../include/flashvm/swf_sound_mix_queue.h#L52): submitted_frames counts every real accepted adapted.write and pump_tail block, played_frames is submitted minus fresh validated queued_frames, and seconds divides this dequeue count by the fixed output rate. Rejected writes count nothing until their exact retained retry succeeds. The maximum exact count is [FVM_SWF_SOUND_OUTPUT_MAX_CLOCK](../include/flashvm/swf_sound_output.h#L14); overflow is UNSUPPORTED before rendering/writing. This is not a DAC timestamp, source EOF, frame authorization or full A/V synchronization.

Impossible queue growth beyond submitted/max backlog or regression of played position returns ERROR without publishing output or changing the last valid played position. Before begin, info is NOT_FOUND without callbacks. Invalid output size/null/busy is INVALID_ARGUMENT with output preserved. Real backend statuses propagate, thrown callbacks become ERROR. Acknowledged adapted.clear resets the active epoch; failed clear preserves counters/pending, and pause preserves them. All calls require external serialization and live borrowed mixer/backend; hot paths allocate nothing and hold no VM/GC references. Public existing structures remain unchanged; native owner storage queries include the added private fields.

The basic fixture proves opt-in behavior, exact rejection/retry counting, queue dequeue and explicit restart. Production desktop integration, device/platform acceptance and all nine roadmap stages remain open.

v0.940: clock_end explicitly disables accounting after producer quiescence, no pending retry and validated FIFO drain. Inactive/nonempty returns NOT_FOUND, pending/null/busy INVALID_ARGUMENT, backend errors propagate. A valid nonempty poll may advance the last played observation while leaving accounting active. No device pause/clear, voice/source reset or ownership transfer is inferred. Tests verify rejected pending/end, unplayed/end, drained/end, legacy count forwarding after end and a fresh explicit begin.

v0.941: callbacks attempt recursive begin/end/info during queue observation, write, pause and clear; every attempt rejects before state publication. Failed begin remains inactive; failed writes/clear/pause and thrown write/queued callbacks preserve retry/accounting. Impossible growth and played regression leave caller snapshots and the last valid position intact. Acknowledged clear resets the active epoch. The ordinary test covers exceptions; --no-throw supports separate sanitizer selection.

v0.942: an explicit white-box seed tests MAX_CLOCK-2, a rejected two-frame block, exact successful retry at MAX_CLOCK and base/tail overflow rejection without voice advance or backend write. After acknowledged clear a real tail starts at the next source sample and counts from zero. The test compiles the actual adapter implementation and seeds only counters; it does not claim real-duration playback of 2^53 frames.

v0.943: a real converted SWF stream producer contributes nine accepted frames; rejected tail output leaves that count unchanged, then exact tail retries bring the shared total/dequeue to 15 frames. The stream-only producer still reports its own nine frames. Frame-gate, source EOF and queue depletion remain distinct. The full stereo rational PCM oracle and independent source VM teardown are retained; drained clock_end succeeds. This proves native handoff accounting, not frontend A/V synchronization.

v0.944: actual MovieClip frame actions execute in interpreter and available native JIT modes with native-entry counters asserted. Shared accounting follows authorized stream frames, stopped ticks/gaps and GC safe points. Explicit goto/stream seek clears through the adapter and starts a matching zero epoch; replayed source bytes and final submitted/dequeued values are checked. Both modes ran in the native JIT profile. This is bounded root timeline/seek coordination, not complete Flash seek or child-clip scheduling.

v0.945: the instrumented ordinary-build allocation test still rejects insufficient adapter byte budget before any new/new[] attempt and checks all three acquisition boundaries. With allocation forbidden, explicit clock begin/info/end/restart, tail rejection/retry, base acceptance, dequeue, pause and clear make zero allocation attempts. Updated private counters are included in required/observed fixed storage without growing hot-path buffers.

v0.946: four actual OS workers independently perform 64 shared-clock epochs each after source VM teardown. Every block is rejected/retried, exact PCM remains independent, submitted=8 with queued=2 reports played=6, simulated consumer drain reports played=8 and then clock_end succeeds. Owners/templates/mixers/backend state are never shared; no new shared-handle thread-safety promise is made.

v0.948: three actual native stream segments preserve the shared accepted count across continuous binding without clearing FIFO. Staged discontinuity/rebind resets the epoch only on acknowledged clear; failed clear preserves old converter binding, source pending PCM, BASE ownership and shared counts. The subsequent successful binding accepts into a fresh epoch, while an EOF binding resets it again. The ordinary test forbids allocation throughout producer transitions; no automatic event-voice seek/replay is inferred.

v0.949: the real C89 fixture creates a bounded mono adapter, begins shared accounting, accepts PCM, observes dequeue/seconds and ends only after drain. Complete caller snapshots are compared byte-for-byte after null/inactive/wrong-size errors. All three new functions link in the minimal NO_MEDIA/NO_FONTS profile; no public existing structure or queue backend ABI is changed.

Host integration boundary: the shared clock observes all accepted composition writes but does not rewrite [FVM_SWFStreamOutputInfo](../include/flashvm/swf_stream_output.h#L11) or merge producer state. Once base EOF is confirmed and tails begin, quiesce the old stream producer and read the shared clock rather than polling/resuming its stream-only clock as if it covered tails. A later discontinuity/rebind requires coordinated pause/clear/source selection; event voices are not replayed or repositioned by shared accounting. Software FIFO drain and DAC/device presentation remain separate.

v0.950 evidence: 113/113 native Release NO_MEDIA/NO_FONTS sound/stream/channel/stop-binding regression and tutorial checks; 25/25 targeted ASan/UBSan checks plus five explicit --no-throw queue/control/stream runs. Native JIT root timeline acceptance ran separately with interpreter=1 native_jit=1 and real native-entry assertions. Exceptions, instrumented allocation sweeps and segmented no-allocation transitions use ordinary builds; LeakSanitizer is disabled because /proc access is unavailable, so no leak-check claim is made. A renamed detached component configures with its own minimal profile and its copied PCM checker passes against the native executable; it was not separately built. Source links and frontend/example catalog checks pass. This series adds native accounting, not device acceptance, complete frontend mixed playback, full A/V/seek/reference parity, child timelines or completion of unrelated graphics/text/AS/GC/debugger stages. All nine roadmap stages remain open.

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
| `FVM_SWFStreamOutputInfo` | struct | [include/flashvm/swf_stream_output.h:11](../include/flashvm/swf_stream_output.h#L11) |
<!-- source-links:end -->
