# Конечный событийный звук без stream

[`fvm_swf_sound_output_pump_finite`](../include/flashvm/swf_sound_output.h#L133) ограничивает свежий PCM живым остатком микшера; pending retry всех режимов, включая stop_release, сохраняется целиком. Нулевые source-отсчёты и release endpoint не удаляются. accepted сначала обнуляется; отсутствие будущих голосов даёт NOT_FOUND без синтеза тишины. Владение, сериализация backend, часы и статусы/исключения совпадают с прежним pump; аллокаций, VM и GC нет. Старый pump продолжает silent priming. NOT_FOUND не означает FIFO drain или физическую слышимость.

v0.1025: Добавлен конечный event-only pump с точными последними кадрами и сохранением legacy/release/pending retry.


v0.1026: Добавлено сериализованное наблюдение voice/retry/FIFO с обновлением dequeue clock и сохранением out при ошибке.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_output_pump_finite` | function | [include/flashvm/swf_sound_output.h:133](../include/flashvm/swf_sound_output.h#L133) |
<!-- source-links:end -->
