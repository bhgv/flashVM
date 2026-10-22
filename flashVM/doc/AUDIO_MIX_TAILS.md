# Event tails through the composition queue

v0.927 adds [fvm_swf_sound_mix_queue_pump_tail](../include/flashvm/swf_sound_mix_queue.h#L93)(queue, frames, target, accepted). After the stream/base producer is quiescent, the host authorizes one zero-base event-only block. frames is positive and at most the fixed adapter capacity; target is at least frames and at most [FVM_SWF_SOUND_OUTPUT_MAX_QUEUE](../include/flashvm/swf_sound_output.h#L12). No clock, frame deadline, pause, seek or EOF transition is inferred.

The pump observes actual queued frames before rendering. Insufficient room or no active voices/pending tail returns NOT_FOUND. Observation errors and backpressure consume no voice time. Successful final blocks may include padding silence; exhaustion does not mean that accepted audio has drained. accepted is optional, cleared first and receives frames only after real acceptance.

A rejected tail write retains exact mixed bytes and already-consumed voice time. Retry the same frame count. Pending base PCM or a different tail size is INVALID_ARGUMENT before callbacks. Ordinary adapted writes cannot consume a tail retry even when their bytes are all zero. Acknowledged clear cancels pending ownership without rewinding voices; pause preserves it. Backend errors propagate, exceptions become ERROR, recursive callbacks are INVALID_ARGUMENT.

No allocation, VM or GC references are used on the pump path. Retain adapter, mixer and backend userdata; externally serialize all operations and quiesce producers before close. Existing public structures and stream callback contracts remain unchanged. The deterministic fixture outlives its source VM, checks exact PCM through rejection/retry, queue backpressure and final padded block. Desktop integration, real devices/platforms and full SWF acceptance remain open.

v0.928: [fvm_swf_sound_mix_queue_pending](../include/flashvm/swf_sound_mix_queue.h#L31) copies an additive exact-size Pending structure with frames and NONE/BASE/TAIL origin. It makes no callback/allocation/VM/GC access, changes no voice time and is not a reservation. Invalid arguments/reentry preserve output. Ownership tests prove both directions of retry separation even for identical zero input and an exhausted mixer; a different tail retry size cannot steal pending output.

v0.929: storage_required predicts checked native object plus two fixed S16 array requests without allocating or acquiring a mixer/device. Channels 1/2 and positive bounded block are required; invalid arguments and size overflow preserve output. storage_info copies fixed_bytes, buffer_bytes and retained_bytes from a live serialized adapter with exact-size output and no callback/VM/GC. Allocator bookkeeping, mixer/voice PCM, real queue and VM memory are excluded. Mono/stereo tests check exact buffer bytes, total agreement and unchanged storage during rejected writes.

v0.930: create_bounded validates the original descriptors and matching mixer geometry first, then compares the exact fixed requested bytes to budget before allocating. Zero/insufficient budget returns OUT_OF_MEMORY, clears the native owner output and preserves the adapted backend. Exact fit succeeds without any device call. The original create delegates with an unrestricted ceiling. Neither function owns or budgets the borrowed mixer, voices or backend. Original allocation-boundary tests still pass.

v0.931: real backend callbacks attempt recursive pumping, pending observation and clear; all are rejected while busy with preserved observation outputs. Failed queued/pause/clear and a thrown write preserve exact pending tail bytes. Accepted clear discards that block without rewinding voices, so the next accepted PCM begins at the later sample. The ordinary build checks exceptions; --no-throw supports a separately selected sanitizer path.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pending` | function | [include/flashvm/swf_sound_mix_queue.h:31](../include/flashvm/swf_sound_mix_queue.h#L31) |
| `fvm_swf_sound_mix_queue_pump_tail` | function | [include/flashvm/swf_sound_mix_queue.h:93](../include/flashvm/swf_sound_mix_queue.h#L93) |
| `FVM_SWF_SOUND_OUTPUT_MAX_QUEUE` | macro / define | [include/flashvm/swf_sound_output.h:12](../include/flashvm/swf_sound_output.h#L12) |
<!-- source-links:end -->
