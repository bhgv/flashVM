# Desktop root stream/event composition


v0.975: Add [fvm_swf_stream_output_pump_mix_tail](../include/flashvm/swf_stream_output.h#L82) for final EOF: exact retry and coherent accepted/dequeued counters; no public struct change.


v0.976: Verify exact backend pairing, rejected/throwing retry, stream reentry and independently advanced epoch rejection.


v0.977: Acquire/close optional native composition ownership and route serialized ID controls; keep mixed preflight closed until integration checks.


v0.978: Allow bounded root mixed acquisition and synchronize navigation before adopting each frame event; preserve legacy event/stream paths.


v0.979: Pump paired tails only at final source EOF, include active/pending events in completion and keep StopSounds-held sources cancelled.


v0.980: Exercise production C89 refill against an explicit software FIFO, retained base retry and exact 5 base-plus-event/4 tail PCM frames.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_output_pump_mix_tail` | function | [include/flashvm/swf_stream_output.h:82](../include/flashvm/swf_stream_output.h#L82) |
<!-- source-links:end -->
