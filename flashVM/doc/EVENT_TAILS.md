# Finite event-only audio output

The opt-in [`fvm_swf_sound_output_pump_finite`](../include/flashvm/swf_sound_output.h#L133) stops fresh PCM at the live mixer horizon, preserving every pending retry, including legacy or stop_release bytes. It does not remove silent-valued source frames or release endpoints. accepted clears first; empty future voices return NOT_FOUND without silent priming. Same owner lifetime, serialized backend callbacks, clock bounds and exception/status contract as the original pump; no allocation, VM or GC. The original pump retains continuous silence behavior. NOT_FOUND is not queue drain or physical audibility.

v0.1025: Add opt-in finite output pumping with exact final frames and unchanged legacy, release and pending retry ownership.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_output_pump_finite` | function | [include/flashvm/swf_sound_output.h:133](../include/flashvm/swf_sound_output.h#L133) |
<!-- source-links:end -->
