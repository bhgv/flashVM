# Event tails through the composition queue

v0.927 adds [fvm_swf_sound_mix_queue_pump_tail](../include/flashvm/swf_sound_mix_queue.h#L48)(queue, frames, target, accepted). After the stream/base producer is quiescent, the host authorizes one zero-base event-only block. frames is positive and at most the fixed adapter capacity; target is at least frames and at most [FVM_SWF_SOUND_OUTPUT_MAX_QUEUE](../include/flashvm/swf_sound_output.h#L12). No clock, frame deadline, pause, seek or EOF transition is inferred.

The pump observes actual queued frames before rendering. Insufficient room or no active voices/pending tail returns NOT_FOUND. Observation errors and backpressure consume no voice time. Successful final blocks may include padding silence; exhaustion does not mean that accepted audio has drained. accepted is optional, cleared first and receives frames only after real acceptance.

A rejected tail write retains exact mixed bytes and already-consumed voice time. Retry the same frame count. Pending base PCM or a different tail size is INVALID_ARGUMENT before callbacks. Ordinary adapted writes cannot consume a tail retry even when their bytes are all zero. Acknowledged clear cancels pending ownership without rewinding voices; pause preserves it. Backend errors propagate, exceptions become ERROR, recursive callbacks are INVALID_ARGUMENT.

No allocation, VM or GC references are used on the pump path. Retain adapter, mixer and backend userdata; externally serialize all operations and quiesce producers before close. Existing public structures and stream callback contracts remain unchanged. The deterministic fixture outlives its source VM, checks exact PCM through rejection/retry, queue backpressure and final padded block. Desktop integration, real devices/platforms and full SWF acceptance remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pump_tail` | function | [include/flashvm/swf_sound_mix_queue.h:48](../include/flashvm/swf_sound_mix_queue.h#L48) |
| `FVM_SWF_SOUND_OUTPUT_MAX_QUEUE` | macro / define | [include/flashvm/swf_sound_output.h:12](../include/flashvm/swf_sound_output.h#L12) |
<!-- source-links:end -->
