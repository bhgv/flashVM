# Adaptive event-only queue filling

Fresh event-only audio may fill partial FIFO headroom instead of waiting for a
whole render block. [`fvm_swf_sound_mix_queue_pump_tail_up_to`](../include/flashvm/swf_sound_mix_queue.h#L262) takes a live native
queue, a maximum block size, a positive target backlog and optional accepted count.
The maximum cannot exceed the immutable queue capacity. The target may be smaller
than the maximum. With active voices it renders min(maximum, target minus observed
backlog); a full queue or no voices returns NOT_FOUND without rendering.

A failed write owns an indivisible TAIL retry. A smaller maximum or insufficient
headroom returns NOT_FOUND; neither splits nor remixes it. A later call with enough
room resends its original size and bytes, even if the maximum changed. Pending BASE
input is INVALID_ARGUMENT. The original exact-size [`fvm_swf_sound_mix_queue_pump_tail`](../include/flashvm/swf_sound_mix_queue.h#L246)
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


v0.1000: Use adaptive tails in paired EOF and frame-gated scheduling while retaining short TAIL priority over newly permitted base.


v0.1001: Carry the final pre-write FIFO observation into paired counters without a fallible post-acceptance callback.


v0.1002: Cover backend failures, callback reentry, exceptions, BASE exclusion and acknowledged cancellation of short retries.


v0.1003: Exercise the last representable single frame, overflow rejection, invalid backlog and acknowledged epoch reset.


v0.1004: Reject all native allocations after preparation across short writes, retry, gate changes, event stop and seek clear.


v0.1005: Check 256 source-teardown epochs with one-frame FIFO drain, rejected short tails, opening gates and exact PCM and ID expiry.


v0.1006: Exercise production C89 worker scheduling in interpreter and native JIT with GC, short retry, exact event PCM and retained base resume.


v0.1007: Add an event-only headroom mode with fourteen one-frame rejected blocks, unchanged source position and shared ID clock drain.


v0.1008: Check all eight export modes including exact fourteen-frame headroom PCM, rejected retries and queued-to-drained identity lifetime.


## Paired and desktop scheduling

[`fvm_swf_stream_output_pump_mix_until`](../include/flashvm/swf_stream_output.h#L111) and [`fvm_swf_stream_output_pump_mix_tail`](../include/flashvm/swf_stream_output.h#L92) use
adaptive tails while preserving their existing gate/observed-EOF preconditions.
Short retained tails take priority over new base. The stream's base PCM pending
field remains base-only; query mix pending for event retry ownership. Source cursor,
phase, chunk and EOF remain unchanged during event-only work. Both clock copies
use the FIFO observation immediately before the successful write; no fallible
callback follows acceptance. A backend can consume during later callbacks, so
this is a consistent admission observation, not a physical DAC timestamp.

The desktop root composition worker inherits this behavior during future frame
gates, authored root Stop and source EOF. Host pause continues to suspend complete
output. BASE retries are never shortened or substituted; accepted PCM cannot be
selectively recalled. Chosen final padding depends on block/headroom geometry;
one-frame filling can retire exactly at event duration. This adds no automatic
A/V drift correction, child timeline scheduling or reference seek preservation.


v0.1009: Specify partial FIFO sizing, indivisible retries, admission-clock snapshots, desktop pause policy and eight-mode tutorial limits.


v0.1010: Check exact changed final padding and segment offsets when one-frame headroom alters tail block boundaries.


## v0.1011 verification / Проверка

Adaptive event tails v0.999–v0.1011: the additive pump_tail_up_to fills fresh event-only blocks into partial FIFO headroom while preserving indivisible short retries. Paired EOF/gate and desktop root composition use it; the exact-size API and BASE retry ownership remain unchanged. Stream counters now carry the actual successful admission observation without a fallible post-write poll. Tests cover one-frame dequeue, changed retry caps, controls/errors/reentry, seeded exact clock limits, zero allocation after preparation, 256 epochs on four independent OS workers and real root Stop/Play/GC with interpreter/native JIT entries. Four existing desktop PCM oracles were updated for changed final padding and segment offsets, with exact sample checks. Final validation: 180 native regression/tutorial/CLI checks, 43 selected ASan/UBSan checks plus ten explicit --no-throw runs, and eight SDK-free core/tutorial checks pass. Ordinary builds cover exceptions/allocation faults; LeakSanitizer is disabled. The C89 exporter has eight modes with an independent rational PCM oracle. A renamed detached component configures with an explicit SDL SDK; its copied checker passes against the native executable, without a separate detached build. SDL is dummy-only. Child dictionaries/timelines, full codec/reference latency, sample-clock A/V/drift/seek, device/platform acceptance and all nine stages remain open.


v0.1011: Record partial FIFO scheduling, coherent admission clocks, exact PCM evidence and the remaining SWF and device acceptance boundaries.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pump_tail` | function | [include/flashvm/swf_sound_mix_queue.h:246](../include/flashvm/swf_sound_mix_queue.h#L246) |
| `fvm_swf_sound_mix_queue_pump_tail_up_to` | function | [include/flashvm/swf_sound_mix_queue.h:262](../include/flashvm/swf_sound_mix_queue.h#L262) |
| `fvm_swf_stream_output_pump_mix_tail` | function | [include/flashvm/swf_stream_output.h:92](../include/flashvm/swf_stream_output.h#L92) |
| `fvm_swf_stream_output_pump_mix_until` | function | [include/flashvm/swf_stream_output.h:111](../include/flashvm/swf_stream_output.h#L111) |
<!-- source-links:end -->
