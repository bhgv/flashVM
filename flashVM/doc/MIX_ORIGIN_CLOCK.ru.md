# Раздельный dequeue-учёт BASE/TAIL

[`fvm_swf_sound_mix_queue_storage_required_for_modes`](../include/flashvm/swf_sound_mix_queue.h#L60) рассчитывает native owner/PCM и опциональные ID/BASE-TAIL bitmap. Оба флага Boolean; неверные флаги/геометрия дают INVALID_ARGUMENT с сохранением out, переполнение размера — OUT_OF_MEMORY. Аллокаций/callback/VM/GC нет. Bitmap добавляет 32768 байт для FIFO до 262144 кадров; mixer/voice/backend/allocator/VM исключены.

v0.1038: Добавлен точный расчёт памяти BASE/TAIL origin-учёта без изменения смысла прежних predictors.


v0.1039: Добавлены фиксированный origin bitmap, проверенный dequeue и точные BASE/TAIL счётчики; legacy callbacks сохранены.


v0.1040: Проверены TAIL-классификация retry, stop/failed clear, неверный рост FIFO, рекурсия и исключения callbacks.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_storage_required_for_modes` | function | [include/flashvm/swf_sound_mix_queue.h:60](../include/flashvm/swf_sound_mix_queue.h#L60) |
<!-- source-links:end -->
