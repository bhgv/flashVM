# Event tails through the composition queue

For one accepted/dequeued epoch across base and tail output, use the [shared clock](MIX_QUEUE_CLOCK.md). It does not merge the old stream producer counters or supply timeline authorization. Quiesce the old producer before tails; use the shared clock thereafter and coordinate any later rebind/seek through acknowledged clear.

v0.927 adds [fvm_swf_sound_mix_queue_pump_tail](../include/flashvm/swf_sound_mix_queue.h#L214)(queue, frames, target, accepted). After the stream/base producer is quiescent, the host authorizes one zero-base event-only block. frames is positive and at most the fixed adapter capacity; target is at least frames and at most [FVM_SWF_SOUND_OUTPUT_MAX_QUEUE](../include/flashvm/swf_sound_output.h#L12). No clock, frame deadline, pause, seek or EOF transition is inferred.

The pump observes actual queued frames before rendering. Insufficient room or no active voices/pending tail returns NOT_FOUND. Observation errors and failed pre-render headroom checks consume no voice time. Successful final blocks may include padding silence; exhaustion does not mean that accepted audio has drained. accepted is optional, cleared first and receives frames only after real acceptance.

A rejected tail write retains exact mixed bytes and already-consumed voice time. Retry the same frame count. Pending base PCM or a different tail size is INVALID_ARGUMENT before callbacks. Ordinary adapted writes cannot consume a tail retry even when their bytes are all zero. Acknowledged clear cancels pending ownership without rewinding voices; pause preserves it. Backend errors propagate, exceptions become ERROR, recursive callbacks are INVALID_ARGUMENT.

No allocation, VM or GC references are used on the pump path. Retain adapter, mixer and backend userdata; externally serialize all operations and quiesce producers before close. Existing public structures and stream callback contracts remain unchanged. The deterministic fixture outlives its source VM, checks exact PCM through rejection/retry, queue backpressure and final padded block. Desktop integration, real devices/platforms and full SWF acceptance remain open.

v0.928: [fvm_swf_sound_mix_queue_pending](../include/flashvm/swf_sound_mix_queue.h#L31) copies an additive exact-size Pending structure with frames and NONE/BASE/TAIL origin. It makes no callback/allocation/VM/GC access, changes no voice time and is not a reservation. Invalid arguments/reentry preserve output. Ownership tests prove both directions of retry separation even for identical zero input and an exhausted mixer; a different tail retry size cannot steal pending output.

v0.929: storage_required predicts checked native object plus two fixed S16 array requests without allocating or acquiring a mixer/device. Channels 1/2 and positive bounded block are required; invalid arguments and size overflow preserve output. storage_info copies fixed_bytes, buffer_bytes and retained_bytes from a live serialized adapter with exact-size output and no callback/VM/GC. Allocator bookkeeping, mixer/voice PCM, real queue and VM memory are excluded. Mono/stereo tests check exact buffer bytes, total agreement and unchanged storage during rejected writes.

v0.930: create_bounded validates the original descriptors and matching mixer geometry first, then compares the exact fixed requested bytes to budget before allocating. Zero/insufficient budget returns OUT_OF_MEMORY, clears the native owner output and preserves the adapted backend. Exact fit succeeds without any device call. The original create delegates with an unrestricted ceiling. Neither function owns or budgets the borrowed mixer, voices or backend. Original allocation-boundary tests still pass.

v0.931: real backend callbacks attempt recursive pumping, pending observation and clear; all are rejected while busy with preserved observation outputs. Failed queued/pause/clear and a thrown write preserve exact pending tail bytes. Accepted clear discards that block without rewinding voices, so the next accepted PCM begins at the later sample. The ordinary build checks exceptions; --no-throw supports a separately selected sanitizer path.

v0.932: an instrumented ordinary-build allocation sweep counts new/new[] attempts. Insufficient byte budget makes zero attempts; each of the three owner/array allocation boundaries fails cleanly, then exact-fit acquisition succeeds. With allocation forbidden, rejected/accepted tail writes, pending/storage observations, pause and clear perform zero attempts and preserve exact samples after source VM teardown. This is a native allocation test, separate from ASan/UBSan.

v0.933: an actual root stream-output producer and two converted event voices run after VM teardown. A zero frame gate holds without cursor movement and is explicitly not EOF. Only observed source EOF with no pending source block authorizes handoff. Rejected first tail retains exact mixed bytes, then a rational PCM oracle verifies the complete 15-frame stream/tail result including one final padding frame. The stream producer still reports only its nine accepted base frames: tail pumping supplies no shared timeline/device clock. Production frontend handoff remains separate.

v0.934: four actual OS workers each own an independent native voice template, mixer, bounded adapter and backend. After destroying each source VM/runtime, every worker performs 64 clone/adopt/render cycles with rejection and exact retry of every two-frame tail block. Independent PCM is verified throughout. No mutable VM, voice, mixer or queue is shared; this is concurrency acceptance for separate owners, not a new shared-state thread-safety promise.

v0.937: immediate stop-by-ID preserves already mixed pending PCM, while stop-all plus acknowledged clear discards it. An independent 44100-Hz oracle checks all 220 five-millisecond DECLICK release samples, final zero and block padding; tail pumping continues releasing voices until exhaustion. Invalid frame/target geometry and impossible backend counts consume no voice time. The C89 link test covers every new API, cleared accepted output and preserved observation/backend outputs.

Host completion sequence: quiesce the base producer and resolve any BASE retry through that producer or acknowledged clear; distinguish source EOF from frame gates and queue backpressure. Pump only explicitly authorized tail frames with a bounded queue target. On NOT_FOUND inspect pending origin: a rejected backend write may own a TAIL block and consumed voice time, so retry that same size. With no pending block, active voices may still be waiting for queue headroom; voice exhaustion and FIFO drain are separate observations. Stop/seek must coordinate mixer controls, pending/accepted PCM and converter state under the host lock; this API supplies no cross-owner transaction or implicit timeline authorization.

v0.938 evidence: 103/103 native Release NO_MEDIA/NO_FONTS sound/stream/channel/stop-binding regression and tutorial checks; 19/19 targeted ASan/UBSan checks plus three explicit --no-throw queue/control/actual-stream executions. Ordinary builds run callback exceptions and allocation sweeps, including the instrumented bounded-tail test. LeakSanitizer is disabled because /proc access is unavailable; no leak-check claim is made. The renamed detached component configures with its own minimal profile and copied PCM checker passes against the native executable, without a separate detached build. Source links and the frontend/example catalog checker pass. No physical audio, production desktop mixed playback, child-clip scheduling, new codec support, A/V/seek conformance or unrelated text/JIT/debugger stage is certified by this series. All nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pending` | function | [include/flashvm/swf_sound_mix_queue.h:31](../include/flashvm/swf_sound_mix_queue.h#L31) |
| `fvm_swf_sound_mix_queue_pump_tail` | function | [include/flashvm/swf_sound_mix_queue.h:214](../include/flashvm/swf_sound_mix_queue.h#L214) |
| `FVM_SWF_SOUND_OUTPUT_MAX_QUEUE` | macro / define | [include/flashvm/swf_sound_output.h:12](../include/flashvm/swf_sound_output.h#L12) |
<!-- source-links:end -->
