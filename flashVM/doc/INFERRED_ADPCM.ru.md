# Подготовка коротких ADPCM-блоков по геометрии байтов

`fvm_swf_stream_sound_snapshot_open_pcm_inferred(context, stream, budget_bytes, out)`
добавляет явную bounded-политику подготовки root stream ADPCM. Desktop-плеер
использует её до открытия устройства и публикации worker. Старые open_pcm и
open_pcm_bounded сохраняют whole-packet поведение. Counted API по-прежнему
принимает внешние точные длины, включая неоднозначные блоки.

## Политика длины

Сначала сохраняется legacy-интерпретация целых пакетов по 4096 кадров. Если она
возвращает UNSUPPORTED, проверяются все возможные окончания с 0–7 нулевыми padding
битами. После двух width-битов учитываются целые пакеты, затем predictor/index
первого кадра хвоста и целое число межканальных differential codes. Между пакетами
нет byte alignment. Одна длина принимается, несколько дают UNSUPPORTED, ни одной —
INVALID_ARGUMENT. После выбора decoder проверяет и декодирует весь блок.

Для canonical whole packets сохраняется прежний результат, даже если возможна
более короткая геометрия. Новый partial-fallback не выбирает длину по average,
FPS, ordinal, ближайшему или самому длинному кандидату. Пустые и корректные
width-only блоки сохраняют прежнюю нулевую длину. Нулевые differential codes
остаются данными, когда входят в единственную геометрию: обрезать все нули нельзя.
Байты не доказывают намерение автора; некоторые усечения сами могут быть валидным
блоком другой длины.

## Владение, ошибки и бюджет API

Требуются живой context на owner-потоке, zero-based root stream, peak-byte ceiling
и отдельный writable snapshot-pointer. Out обнуляется до другой валидации. Во
время синхронной подготовки context/исходные байты стабильны. Успех даёт независимый
immutable normalized native S16 snapshot, сохраняя rate/channels, frame/block
identity, playback metadata и advisory average. Исходный movie не переписывается.
Для остальных поддержанных PCM/MP3 действует прежний bounded-контракт.

Объект, индекс, encoded bytes и decoded PCM совместно входят в peak ceiling.
При нескольких сегментах вычитайте retained bytes предшественников. Нулевого
бюджета недостаточно даже пустому владельцу. Переполнение, native allocation
failure, runtime payload/index limit и превышение peak дают OUT_OF_MEMORY.
Неверные аргументы/источник/представление и missing stream передают статусы
прежнего bounded API; геометрия/неоднозначность описаны выше. Ошибка освобождает
временных владельцев, частичный snapshot не публикуется, audio endpoint не открыт.

Подготовка выделяет память и декодирует до публикации worker, вне audio callback.
Нет script/device callback или исполнения VM; managed/GC pointers не остаются.
После возврата исходную VM можно уничтожить. Readers/converters заимствуют
snapshot; разные курсоры могут читать immutable данные в OS-потоках. Внешне
сериализуйте source mutation и close; закрывайте после остановки всех читателей.
Новых блокировок, общих курсоров и скрытого worker API не создаёт.

## Desktop и native-потребители

prepare_segment использует inferred-политику в существующем aggregate retained/peak
бюджете до audio acquisition. Неоднозначный/невалидный звук отклоняется preflight;
явный mute сохраняет свою политику. Нормализованный PCM поступает в stream-only
и root stream/event mixing. Контракты retry, frame gates, finite tails, StopSounds,
seek-clear epochs, source grid и dequeue clock не меняются. Автоматическая A/V
политика не добавляется.

C89 swf_stream_adpcm --inferred не читает block_frames: последние ненулевые коды
оставляют длины 5 и 3 вокруг пустого блока. Все три режима экспортируют те же
восемь ручных samples после уничтожения VM. [Пример](../examples/multimedia/swf_stream_adpcm/README.md).

## Проверки и незакрытая приёмка

Матрица 104 случаев: ширины 2–5, mono/stereo и длины около 4096/8192; 65 принятых
случаев совпадают по всем samples, 39 неоднозначных отклонены. Oracle независимо
перебирает длины. Проверены плохие окончания/короткие predictors, точный и
one-byte-short бюджет, четыре allocation failures, чтение без аллокаций, VM
teardown, точный retry PCM, закрытый gate, failed clear и успешный seek reset.
Четыре OS-worker читают один immutable snapshot разными converters в 128 эпохах,
сверяя рациональную интерполяцию на restart.

Настоящие SDL dummy CLI проверяют полный PCM FWS/CWS, interpreter/native JIT,
пустые блоки, partial restart и PCM successor. Mixed fixtures проверяют partial
stereo ADPCM с противоположными events, одно насыщение, BASE19/TAIL0 в двух finite
политиках. Это Linux native функциональные проверки, не физический DAC, не race
detector и не приёмка других платформ.

Неоднозначный ADPCM требует авторитетных длин. Дочерние таймлайны/словари,
reference codec latency/seek, source time, автоматический A/V drift и полная
SWF <=8 совместимость остаются открытыми. Все девять стадий открыты.

## Independent codec oracle / Независимый codec oracle

Run `python3 scripts/check_inferred_adpcm_ffmpeg.py BUILD/flashvm_tests_swf_stream_adpcm_inferred`.
The test exporter receives only SWF/output paths, never a count table. Varying
codes exercise every width/channel pair, partial restarts, empty/width-only
blocks and FWS/CWS. FFmpeg is an optional test executable, not a runtime dependency.
Fixture lengths come from independent construction and endpoint enumeration.

FFmpeg используется только как внешний oracle. Экспортёр FlashVM не получает
таблицу длин; полный PCM сверяется для всех ширин/каналов и обоих контейнеров.

Inferred partial ADPCM v0.1064–v0.1076: новый bounded API сохраняет canonical whole-packet поведение и принимает единственную partial-геометрию байтов/padding без header average и movie time. Старые whole-only и counted API сохраняют политику; неоднозначные короткие блоки автоматически не принимаются. Desktop готовит stream-only и mixed источники этим API в общем retained/peak бюджете до устройства. Прошли 223 выбранных native regression/tutorial/CLI проверки и две options-проверки; 78 full-profile ASan/UBSan, четырнадцать --no-throw запусков и isolated inferred allocation-success; 14 SDK-free ADPCM проверок; независимое сравнение PCM с FFmpeg всех ширин/каналов в FWS/CWS. Четыре allocation failures проверены обычной сборкой; LeakSanitizer отключён. Матрица 104 случаев принимает 65 и отклоняет 39 неоднозначных. Проверены плохие окончания, точные бюджеты, VM teardown, gates, точный retry, failed clear и seek/reset. Четыре настоящих OS-worker сверяют рациональную интерполяцию на restart в 128 эпохах. SDL dummy CLI сверяет весь PCM, пустые блоки, PCM successor, обе VM modes, opposite events, finite policies и BASE19/TAIL0. C89 exporter проверяет восемь ручных samples в counted/range/inferred режимах. Переименованный detached-компонент конфигурируется с явным SDK; скопированные FFmpeg/tutorial checkers проходят на native binaries, отдельной detached-сборки нет. Неоднозначные длины, дочерние таймлайны/словари, reference codec latency/seek, автоматические source-time/A-V drift, устройства/другие платформы и все девять стадий остаются открытыми.


v0.1064: Добавлена bounded-подготовка с прежними whole packets и единственной partial-длиной без header average.

v0.1065: Для 104 геометрий независимо перебираются длины и проверяется каждый PCM sample, неоднозначность и legacy whole packets.

v0.1066: Проверены четыре native-аллокации, точный и недостаточный бюджет, уничтожение VM и чтение PCM без аллокаций.

v0.1067: Проверены неверные окончания, короткий predictor, неоднозначный mono и единственное nonzero-окончание с точным PCM.

v0.1068: Desktop-сегменты используют inferred bounded snapshot с общим peak-бюджетом и ошибкой до открытия устройства.

v0.1069: Проверены FWS/CWS, interpreter/JIT, точный PCM, restart, пустые блоки, PCM successor и отказ до устройства.

v0.1070: Worker проверяет single-saturation stereo PCM и BASE19/TAIL0 для обоих finite-режимов, CWS/FWS и двух VM modes.

v0.1071: C89-пример добавляет inferred-режим без таблицы; все три режима проверяются по восьми ручным samples.

v0.1072: Четыре OS-worker читают общий post-VM snapshot с разными курсорами, проверяя интерполяцию и restart в 128 эпохах.

v0.1073: Post-VM PCM сохраняется при rejected write, gate, failed clear и reset; финальный short retry даёт ровно 19 stereo frames.

v0.1074: Описаны canonical compatibility, unique partial endpoints, counted API, peak budget и границы native/physical приёмки RU/EN.


v0.1075: Добавлены post-VM export без counts и FFmpeg oracle для всех ширин/каналов, CWS/FWS, partial restart и пустых блоков.


v0.1076: Зафиксированы native, sanitizer, SDK-free, FFmpeg и desktop PCM проверки; неоднозначные длины, A/V и дочерние потоки остаются открытыми.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_sound_snapshot_open_pcm_inferred` | function | [include/flashvm/swf_stream_sound.h:136](../include/flashvm/swf_stream_sound.h#L136) |
<!-- source-links:end -->
