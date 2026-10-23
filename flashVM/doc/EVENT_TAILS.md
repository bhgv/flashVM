# Finite event-only audio output

The opt-in [`fvm_swf_sound_output_pump_finite`](../include/flashvm/swf_sound_output.h#L133) stops fresh PCM at the live mixer horizon, preserving every pending retry, including legacy or stop_release bytes. It does not remove silent-valued source frames or release endpoints. accepted clears first; empty future voices return NOT_FOUND without silent priming. Same owner lifetime, serialized backend callbacks, clock bounds and exception/status contract as the original pump; no allocation, VM or GC. The original pump retains continuous silence behavior. NOT_FOUND is not queue drain or physical audibility.

v0.1025: Add opt-in finite output pumping with exact final frames and unchanged legacy, release and pending retry ownership.


v0.1026: Add a serialized pending observation with coherent dequeue clock updates and preserved caller output on errors.


v0.1027: Verify failed clear and write ownership, exception containment, callback reentry and anonymous release completion after voices retire.


v0.1028: Check NoMultiple through dequeue, the last exact clock frame and zero allocations across short rejected retries and cancellation.


v0.1029: Verify 256 epochs with exact short retries, independent rational PCM, coherent pending observations and queued identity expiry.


v0.1030: Add immutable event policy, explicit finite-audio-tails CLI parsing and compatibility wrappers for previous mixed-only startup.


v0.1031: Hold event-only production during source preparation and frame callbacks, then authorize native work from the owner safe point.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_output_pump_finite` | function | [include/flashvm/swf_sound_output.h:133](../include/flashvm/swf_sound_output.h#L133) |
<!-- source-links:end -->
