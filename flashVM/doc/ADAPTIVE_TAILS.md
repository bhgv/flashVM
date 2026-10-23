# Adaptive event-only queue filling

Fresh event-only audio may fill partial FIFO headroom instead of waiting for a
whole render block. [`fvm_swf_sound_mix_queue_pump_tail_up_to`](../include/flashvm/swf_sound_mix_queue.h#L222) takes a live native
queue, a maximum block size, a positive target backlog and optional accepted count.
The maximum cannot exceed the immutable queue capacity. The target may be smaller
than the maximum. With active voices it renders min(maximum, target minus observed
backlog); a full queue or no voices returns NOT_FOUND without rendering.

A failed write owns an indivisible TAIL retry. A smaller maximum or insufficient
headroom returns NOT_FOUND; neither splits nor remixes it. A later call with enough
room resends its original size and bytes, even if the maximum changed. Pending BASE
input is INVALID_ARGUMENT. The original exact-size [`fvm_swf_sound_mix_queue_pump_tail`](../include/flashvm/swf_sound_mix_queue.h#L206)
keeps its existing behavior. accepted is cleared first, then receives actual accepted
frames only on OK. Invalid arguments/reentry are INVALID_ARGUMENT; callback statuses
propagate, exceptions are ERROR, and exact shared-clock overflow is UNSUPPORTED
before rendering. Successful observations may refresh played time on non-success.

The host must quiesce base production and externally serialize queue, mixer and
exclusive backend. Retain all borrowed owners through callbacks. No allocation,
VM, GC, implicit pause, clear, seek or ownership transfer occurs. Final chosen
blocks can contain padding; this API does not infer source EOF or physical audibility.
Use the paired stream scheduler when sharing stream accepted-frame accounting.
Child timelines/dictionaries and reference A/V, codec, seek and device acceptance
remain open, as do all nine roadmap stages.

v0.999: Add bounded adaptive tail filling with indivisible retained retries and unchanged exact-size API.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pump_tail` | function | [include/flashvm/swf_sound_mix_queue.h:206](../include/flashvm/swf_sound_mix_queue.h#L206) |
| `fvm_swf_sound_mix_queue_pump_tail_up_to` | function | [include/flashvm/swf_sound_mix_queue.h:222](../include/flashvm/swf_sound_mix_queue.h#L222) |
<!-- source-links:end -->
