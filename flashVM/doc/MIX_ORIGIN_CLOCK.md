# Mixed queue BASE/TAIL dequeue accounting

[`fvm_swf_sound_mix_queue_storage_required_for_modes`](../include/flashvm/swf_sound_mix_queue.h#L60) predicts requested native owner/PCM memory with optional ID tracking and a fixed BASE/TAIL FIFO bitmap. Both flags are Boolean; invalid geometry/flags return INVALID_ARGUMENT preserving out, checked size overflow OUT_OF_MEMORY. No allocation/callback/VM/GC occurs. Origin storage adds 32768 bytes for the maximum 262144-frame queue; mixer, voice, backend, allocator and VM storage are excluded.

v0.1038: Add exact mode-aware native storage prediction for optional accepted-frame origin accounting without changing legacy predictor meaning.


v0.1039: Add opt-in fixed bitmap origin accounting, validated FIFO retirement and exact category clock observations while preserving legacy callbacks.


v0.1040: Verify retained TAIL classification, stop and failed-clear ownership, invalid queue growth, callback recursion and exception containment.


v0.1041: Exercise wraparound, bounded admission before render and the last exact epoch frames using explicit seeded native metadata.


v0.1042: Verify paired BASE and TAIL origins and publish the actual successful base admission clock without a fallible post-write callback.


v0.1043: Reject mode allocation transactionally and exercise tracked base/tail retries, observations and acknowledged seek reset with no further allocations.


v0.1044: Verify 256 tracked epochs with exact PCM, held-tail handoff, converted stream EOF and coherent category dequeue totals.


v0.1045: Enable accepted origin accounting in root composition and reject combined ID and bitmap budgets before any SDL endpoint is opened.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_storage_required_for_modes` | function | [include/flashvm/swf_sound_mix_queue.h:60](../include/flashvm/swf_sound_mix_queue.h#L60) |
<!-- source-links:end -->
