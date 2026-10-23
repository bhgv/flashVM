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


v0.1020: Проверены 256 эпох: отклонённый held TAIL, разрешение BASE, точный насыщенный PCM, source EOF и истечение ID.


v0.1021: Девятый C89-режим показывает четыре точных retry, короткий последний TAIL, неизменный BASE и queued/drained ID.


v0.1022: Рациональный PCM-checker проверяет девять режимов, конечный вывод, четыре retry и истечение ID после drain.


## Конечные нативные producer

[`fvm_swf_sound_mix_queue_pump_tail_finite`](../include/flashvm/swf_sound_mix_queue.h#L271) сохраняет адаптивный FIFO-контракт
[`fvm_swf_sound_mix_queue_pump_tail_up_to`](../include/flashvm/swf_sound_mix_queue.h#L262), ограничивая новый zero-base блок живым
остатком микшера. maximum — 1..capacity; target положителен и не выше
SOUND_OUTPUT_MAX_QUEUE. accepted необязателен и сначала обнуляется. Нет голосов,
полный FIFO или недостаточно места для retry — NOT_FOUND; null/reentry/неверная
геометрия/pending BASE — INVALID_ARGUMENT. Переполнение общих часов даёт UNSUPPORTED
до render, ошибки backend передаются, исключения дают ERROR. Существующий TAIL
неделим, даже если создан старым padded API или голоса остановлены после отказа.
Повторяются исходные размер и байты; отмена — только подтверждённый clear. Убирается
синтетический padding, но не авторские нулевые отсчёты и не release endpoint.

[`fvm_swf_stream_output_pump_mix_finite_until`](../include/flashvm/swf_stream_output.h#L114) добавляет этот режим в парный stream
планировщик: проверяет точный backend и общую эпоху, принимает TAIL до нового BASE,
удерживает BASE закрытого кадра и сохраняет source phase при event-only выводе.
Base-конверсия не меняется. Кадр ноль удерживает base, разрешая события. accepted
сначала обнуляется; OK может потребить нулевой marker. NOT_FOUND не означает EOF:
возможны missing clock, headroom, held BASE или исчерпание голосов. Прочие статусы
передаются. Обе копии часов получают наблюдение допуска успешного write без
ошибочного callback после принятия. Все заимствованные stream/converter/snapshot/
queue/mixer/backend живут до завершения и сериализуются извне; аллокаций, VM и GC нет.
Прежние парные/exact/adaptive API сохраняют поведение и ABI.

## Desktop

`flashvm_player --finite-mixed-tails movie.swf` включает конечный root mixed TAIL.
Флаг относится только к stream-plus-event composition; отдельные event-only и
stream-only пути сохраняют прежнюю политику. С --mute устройство не открывается.
`fvm_desktop_audio_open_mixed_policy` проверяет Boolean-флаги и записывает неизменную
политику до запуска worker; старые open выбирают padded режим. Публикация полного
кадра, authored Stop, host pause, StopSounds, seek и queued ID сохраняют контракты.
Короткая конечная очередь возобновляется для drain, даже если меньше driver reserve.

Девятый C89-режим `swf_stream_mix --finite` после удаления VM пишет 14 кадров
блоками 4,4,4,2, отклоняя каждый блок один раз. Независимый рациональный оракул
проверяет PCM, числа и истечение ID. Проверены частоты/каналы/повторы/частичная
adoption, strict/converted release, ограниченная арифметика огромных повторов,
controls/reentry/exception, отсутствие аллокаций и 256 эпох на четырёх независимых
OS-потоках. Production C89 worker проверяет interpreter/native JIT entries и GC;
CLI FWS/CWS Stop/gate/mute использует SDL dummy worker и точные 9/14-frame WAV.
Huge-repeat тест задаёт граничные метаданные, а не выделяет огромный источник.

Дочерние словари/таймлайны, reference codec latency/seek, физические DAC timestamps,
A/V drift, другие backend/платформы/устройства и все девять стадий остаются открытыми.


v0.1023: Уточнены пределы horizon, неделимость retry, source/release, явное включение плеера и открытая reference/device-приёмка.


## v0.1024 verification / Проверка

Конечные mixed-хвосты v0.1012–v0.1024: ограниченный остаток на живой сетке микшера учитывает дробный lookahead, повторы и меньшую source/release-длительность. Новые finite queue/paired API убирают только свежий event-only padding; существующие retry всех режимов остаются неделимыми. Desktop --finite-mixed-tails задаёт неизменную политику до запуска worker; default, event-only и stream-only пути сохраняют прежнее поведение. Root Stop/gates, host pause, StopSounds/seek и queued ID сохраняют владение. Прошли 191 native regression/tutorial/CLI проверка, 51 выбранная ASan/UBSan плюс одиннадцать --no-throw запусков, десять SDK-free core/tutorial и две дополнительные options/help проверки. Четыре старых white-box queue limits пересобраны и повторно проверены; граничные метаданные не означают огромный source/device run. Исключения/ошибки выделений проверены обычной сборкой; LeakSanitizer отключён. Есть независимая conversion/duration/PCM-матрица, strict/converted release, отсутствие аллокаций final retry/cancel и 256 эпох на четырёх независимых OS-потоках. C89 worker подтверждает interpreter/native JIT entries и GC; CLI FWS/CWS Stop/gate даёт точные 9/14-frame WAV и mute на SDL dummy. Девятый C89-режим проверяет хвосты 4,4,4,2 независимым рациональным PCM-оракулом. Отделённый переименованный компонент конфигурируется с явным SDK, скопированный девятирежимный checker проходит на native-бинарнике; отдельной сборки не было. Дочерние словари/таймлайны, полная codec/reference latency/seek, физические DAC/A-V/drift, другие backend/платформы/устройства и все девять стадий остаются открытыми.


v0.1024: Проверены CLI policy и Boolean-валидация; зафиксированы finite mixer/player/tutorial контракты и открытая приёмка.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pump_tail_finite` | function | [include/flashvm/swf_sound_mix_queue.h:271](../include/flashvm/swf_sound_mix_queue.h#L271) |
| `fvm_swf_sound_mix_queue_pump_tail_up_to` | function | [include/flashvm/swf_sound_mix_queue.h:262](../include/flashvm/swf_sound_mix_queue.h#L262) |
| `fvm_swf_sound_mixer_active_frames` | function | [include/flashvm/swf_sound_mixer.h:54](../include/flashvm/swf_sound_mixer.h#L54) |
| `FVM_SWF_SOUND_MIXER_MAX_HORIZON` | macro / define | [include/flashvm/swf_sound_mixer.h:46](../include/flashvm/swf_sound_mixer.h#L46) |
| `fvm_swf_stream_output_pump_mix_finite_until` | function | [include/flashvm/swf_stream_output.h:114](../include/flashvm/swf_stream_output.h#L114) |
<!-- source-links:end -->
