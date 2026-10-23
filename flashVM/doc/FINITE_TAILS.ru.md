# Точное завершение смешанных событийных хвостов

[`fvm_swf_sound_mixer_active_frames`](../include/flashvm/swf_sound_mixer.h#L54) вычисляет ограниченный остаток на текущей сетке микшера: дробная фаза/lookahead, повторы и release. PCM не читается; физическая слышимость не определяется. Владельцы сериализуются извне; аллокаций, callbacks, VM и GC нет. maximum — 1..[FVM_SWF_SOUND_MIXER_MAX_HORIZON](../include/flashvm/swf_sound_mixer.h#L46); неверные параметры дают INVALID_ARGUMENT, сохраняя out. Пустой микшер даёт OK и ноль. Результат ограничен maximum; нулевые отсчёты источника тоже считаются.

v0.1012: Добавлен остаток кадров с учётом текущей дробной фазы, lookahead, повторов и release без чтения PCM.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mixer_active_frames` | function | [include/flashvm/swf_sound_mixer.h:54](../include/flashvm/swf_sound_mixer.h#L54) |
| `FVM_SWF_SOUND_MIXER_MAX_HORIZON` | macro / define | [include/flashvm/swf_sound_mixer.h:46](../include/flashvm/swf_sound_mixer.h#L46) |
<!-- source-links:end -->
