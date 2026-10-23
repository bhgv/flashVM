# Точное завершение смешанных событийных хвостов

[`fvm_swf_sound_mixer_active_frames`](../include/flashvm/swf_sound_mixer.h#L54) вычисляет ограниченный остаток на текущей сетке микшера: дробная фаза/lookahead, повторы и release. PCM не читается; физическая слышимость не определяется. Владельцы сериализуются извне; аллокаций, callbacks, VM и GC нет. maximum — 1..[FVM_SWF_SOUND_MIXER_MAX_HORIZON](../include/flashvm/swf_sound_mixer.h#L46); неверные параметры дают INVALID_ARGUMENT, сохраняя out. Пустой микшер даёт OK и ноль. Результат ограничен maximum; нулевые отсчёты источника тоже считаются.

v0.1012: Добавлен остаток кадров с учётом текущей дробной фазы, lookahead, повторов и release без чтения PCM.


v0.1013: Независимо проверены длительности и PCM: четыре source clock, шесть output rate, каналы, повторы и частично прочитанный voice.


v0.1014: Добавлен конечный TAIL без нового padding; удерживаемые retry всех прежних режимов остаются неделимыми.


v0.1015: Добавлен конечный парный gate/EOF-планировщик с общими часами и сохранением владения BASE/TAIL.


v0.1016: Проверены strict/converted release, ранний EOF источника и ограниченная арифметика огромных повторов без огромных аллокаций.


v0.1017: Добавлен явный finite mixed-tail startup policy и CLI-флаг до публикации worker; прежний режим остаётся по умолчанию.


v0.1018: CLI проверяет точный короткий WAV при root Stop/gates для FWS/CWS в interpreter/JIT и mute без устройства.


v0.1019: Проверены errors/reentry, сохранение pending после stop/ошибки clear и отсутствие нативных аллокаций после подготовки.

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
