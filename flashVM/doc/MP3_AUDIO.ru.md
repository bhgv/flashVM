# MP3-аудио SWF

[English](MP3_AUDIO.md) · [План функциональности](ROADMAP.md)

## v0.776 основа scalar-декодера

Зафиксированный неизменённый [minimp3](../external/minimp3/README.md) Layer III
собирается отдельной static core зависимостью даже при NO_MEDIA. Символы имеют
частные префиксы; scalar C++98 не выбирает ISA во время исполнения, не использует
контейнеры, allocation, VM, callback или device. Публичный host ABI остаётся C89.
Этот первый checkpoint ещё не подключает декодер к публичному чтению SWF resources.

[fvm_swf_mp3_probe](../src/swf/sound_mp3.cpp#L8) проверяет полный непрерывный MPEG-1/2/2.5 Layer-III packet до
публикации bytes/rate/channels и числа кадров 576 либо 1152. Reserved поля,
укороченный packet и мусор перед sync отвергаются; Layer I/II и free-format bitrate
возвращают UNSUPPORTED. Поддержаны CBR и переменные стандартные bitrate indices.
[FVM_SWFMP3Decoder](../src/swf/sound_mp3.hpp#L26) сохраняет reservoir/filter history между пакетами и публикует
PCM после проверки точной геометрии. Decode failure терминален для этого decoder;
поиск sync через мусор и concealment отсутствующего reservoir не выдаются за успех.
Проверка CRC, определение free-format bitrate и полная верификация corrupt Huffman
не заявляются. Отдельные экземпляры владеют всем изменяемым состоянием.

Fixture проверяет три настоящих аналитических MP3: MPEG-2.5 mono 11025, MPEG-2
stereo 22050 и MPEG-1 stereo 44100 Hz. Выбранные отсчёты совпадают с независимым
FFmpeg в пределах двух S16 единиц; проверены полные packet/sample counts, границы
и отказ reserved/free-format fields. Обычная сборка использует включённые vectors
offline. scripts/generate_mp3_test_fixture.py требует FFmpeg/libmp3lame только для
перегенерации аналитических assets и независимых golden samples.

```sh
cmake --build build --target flashvm_tests_swf_mp3_decoder
ctest --test-dir build --output-on-failure -R swf_mp3_decoder
python3 scripts/check_vendor_sources.py
```

Интеграция MP3, event/stream scheduling, seek и native device/platform приёмка —
отдельные условия. Этот decoder checkpoint не закрывает ни одной стадии roadmap.


## v0.777 декодирование DefineSound

[fvm_swf_sound_read_pcm](../include/flashvm/swf_sound.h#L148) теперь читает MP3 в SWF4–8 без allocation/callback. Диапазон
отсчитывается после неотрицательного SI16 SeekSamples. Сначала проверяются все
полные packets; затем новый stack decoder копирует только выбранные кадры.
Объявленная длина должна помещаться после latency; полный encoder tail проверяется,
но не выдаётся сверх sample_count. Скрытой коррекции LAME/Xing gapless нет.
Даже нулевое чтение проверяет весь источник. Изменение/teardown и output buffers
сериализуются owner; managed pointers после возврата не остаются.

[fvm_swf_sound_info](../include/flashvm/swf_sound.h#L134) выдаёт фактические MPEG rate/channels из первого корректного
packet, сохраняя исходные байты и объявленную длину. Advisory SWF flags не вызывают
скрытый resampling. Неверные opaque metadata остаются доступны для анализа, но
decode отвергает данные. Отрицательная latency, free-format и другие MPEG layers
пока UNSUPPORTED. Укороченные/отсутствующие packets, смена geometry и завышенная
длина — INVALID_ARGUMENT; переполнение — OUT_OF_MEMORY. Сохраняются native S16
ограничения. Мелкие повторные чтения заново проверяют источник; для playback нужен
однократно подготовленный voice.

Public fixture проверяет SWF4–8, три MPEG поколения, golden samples до/после
latency, capacity, обрезанный хвост и неизменность выхода, отказ SWF3. Прежние
PCM/event edge checks проходят без изменения ожиданий.

Проверка vendor: новые minimp3 header/LICENSE совпадают с pin. Общий checker
останавливается на прежнем несовпадении AGG agg_array.h с manifest; файл байтово
совпадает с опубликованной v0.775 и не меняется этой серией. Общий vendor pass
не заявляется.

## v0.778 стили MP3 events и native voices

SOUNDINFO позиции в clock 44100 Hz теперь пересчитываются проверяемым целочисленным
разложением для фактических MPEG rates, включая 32/48 kHz. Переполнение возвращает
INVALID_ARGUMENT до extraction/публикации voice вместо wrap на 32-bit unsigned long.
Один helper используется whole-event channel renderer и incremental voices;
обычное поведение nominal PCM сохраняется.

Матрица содержит шесть MPEG geometries 8000–48000 Hz, CBR/VBR. Native voice применяет
in/out interval, два повтора и quarter-gain envelope после MP3 latency. Converted
stereo mixer проверяется независимыми рациональными позициями. До source teardown
выполняется настоящий GC; затем voice/mixer работают без context. Проверены golden
MPEG samples, полные интервалы и native lifetime. Decoder/public-read checks
продолжают проходить. Общий stream/event timeline и reference seek clock не добавлены.

## v0.779 bounded continuous MP3 snapshots

[fvm_swf_stream_sound_snapshot_open_pcm](../include/flashvm/swf_stream_sound.h#L87) и bounded-вариант теперь нормализуют root
MP3 в immutable PCM_NATIVE/S16. Полные Layer-III packets идут в порядке блоков;
один reservoir/filter state проходит через все блоки, включая пустые markers.
Wire SampleCount обязан совпасть с полным decoded MPEG count блока. Фактические
rate/channels заменяют advisory flags. Неотрицательная header latency снимается
один раз через необходимое число начальных packets; SeekSamples первого блока
должен ей соответствовать. Последующие SeekSamples не обрезают continuous PCM.
Отрицательная initial latency и free-format остаются unsupported.

Peak budget включает object/index, encoded bytes и весь decoded PCM. До единственного
PCM allocation проверяется точная длина каждого нормализованного блока; новый
native decoder заполняет ещё не опубликованный результат. При успехе encoded
storage освобождается, при ошибке закрывается всё временное владение. Native stack
state/scratch, allocator bookkeeping и caller buffers не входят в эти charges.
Одновременно действуют runtime payload limit и явный peak ceiling. Reader/converter
работают после VM teardown; live/raw MP3 остаётся encoded. Пустые блоки сохраняют
frame/ordinal identity.

Матрица шести geometries проверяет CBR/VBR, latency 0/1105 с multi-packet trim,
независимые golden values, empty markers, неверные counts, укороченные packets,
точный peak и нехватку одного байта. Бюджеты 0–4 покрывают каждое приобретение и
чтение без allocation после teardown. Прежние counted ADPCM/storage fixtures проходят.

Discontinuous frame seek сохраняет прежнюю prefix-discard PCM policy: исходные
per-block MP3 seek corrections и reference Flash read-ahead/A/V sync ещё не
реализованы. Успешный decode не снимает этих ограничений. Источник формата:
[Adobe SWF specification, sound chapter](https://open-flash.github.io/mirrors/swf-spec-19.pdf).
Указанная приёмка уже полной Flash playback semantics.

## v0.780 интеграция SDL-плеера

Desktop preflight принимает поддержанный MP3 subset вместе с PCM и целым ADPCM.
Каждый сегмент полностью декодируется под прежним aggregate preparation ceiling
до window/device startup. Worker получает immutable native PCM, не VM и не MPEG
decoder. Root StartSound вместе со streams по-прежнему отвергается; event-only MP3
идёт через существующий native event output. Help и source contracts перечисляют
оба пути с ограничениями.

PCM, действительно принятый SDL queue, сравнивается с независимыми FFmpeg/ffprobe
packet boundaries/samples и отдельным рациональным преобразованием в stereo 48000 Hz.
Проходят шесть сочетаний rate/channels, CBR/VBR, FWS/CWS, latency 0/529, настоящие
interpreter/native-JIT action counters и переход MP3→PCM header. Максимальное
отклонение — три S16 единицы, длина обязана совпасть точно. Однокадровый finite run
не включает будущие неразрешённые блоки. Необязательный CTest oracle возвращает
skip 77 без FFmpeg/ffprobe либо native JIT; на этом Linux x64 таких skips не было.

Прежний real worker fixture расширен MP3: медленный owner, frame gate, clip
stop/play, GC safe points, StopSounds, host pause, same-frame navigation token,
cross-header seek и joined clear проверяются в interpreter/JIT modes. Проходят
шесть worker/preparation/budget/refill checks. SDL использует dummy audio/video:
это настоящая queue/runtime проверка, но не прослушивание, DAC clock или foreign
OS/device приёмка.

Latency trim и prefix-discard seek сохраняют описанную native policy. Reference
MP3 seek/read-ahead, negative initial latency, free-format, child streams,
stream/event composition и A/V sync остаются открытыми. Ни одна стадия не закрыта.

## v0.781 исполняемый C89 MP3 exporter

[Построчный tutorial](../examples/multimedia/swf_mp3/README.md) экспортирует первый
MP3 StartSound либо первый root MP3 stream после уничтожения context/runtime.
Input, native preparation и export duration ограничены отдельно; S16 копируется
порциями, native file errors терминальны. В export loop нет device, worker,
resampling, внешнего codec SDK или исходной VM.

Настоящий executable проходит strict C89 и полный minimal-core ASan/UBSan в обоих
режимах, шести MPEG geometries и FWS/CWS. Проходят независимые golden positions,
полные длины, replacement, неверные CLI/source и файловые ошибки. С доступным
FFmpeg сравнивается каждый отсчёт в пределах трёх S16 единиц; без FFmpeg остаются
checked-in golden tests. LeakSanitizer выключен из-за ограничений process enumeration;
ASan injected exception-unwind не засчитывается, как описано ранее.

## v0.782 подготовленные mixer commands без исходной VM

[fvm_swf_sound_mixer_submit_prepared](../include/flashvm/swf_sound_mixer.h#L69)(mixer, voice) принимает указатель на живой
исключительно принадлежащий caller native voice. OK забирает владение и обнуляет
handle; finished/NoMultiple no-op закрывает ненужный owner. Ошибка сохраняет handle,
progress и старые slots. Null — INVALID_ARGUMENT, strict geometry mismatch —
UNSUPPORTED, заполнение — OUT_OF_MEMORY. NoMultiple сначала проверяет активные
не-releasing same-ID voices, как прежняя submission. Фаза conversion начинается
с нуля у ТЕКУЩЕГО voice cursor; частично прочитанный интервал не перематывается.
Нет allocation/callback/VM/managed root. Owner storage не пересекается; доступ
сериализуется снаружи. После transfer handle нельзя читать или закрывать.

[fvm_swf_sound_mixer_stop_id](../include/flashvm/swf_sound_mixer.h#L101)(mixer, sound_id) применяет прежний immediate/DECLICK
stop без context. ID допустим 1–65535; null/неверный ID — INVALID_ARGUMENT до
изменения, отсутствующий ID — OK. Повторный stop не запускает fade заново.
Команды влияют на будущее смешивание: retained retry и accepted device PCM
остаются неизменными до отдельно подтверждённого queue cancellation. Это не
internal lock и не полный frame-command scheduler. Прежний context submit
после owner preparation использует те же native controls.

Allocation fixture запрещает новые allocation после GC/VM teardown и проверяет
transfer, сохранение при заполнении/mismatch, duplicate/empty consumption,
partial-progress adoption, точные следующие отсчёты, неверные/отсутствующие IDs,
immediate и repeated fade stop. C89 MP3 tutorial теперь создаёт mixer и передаёт
ему voice ПОСЛЕ уничтожения VM, затем экспортирует только явную конечную длину.
Независимый golden/full-FFmpeg oracle не менялся. Это static-core C APIs;
автоматический desktop stream/event scheduler остаётся открытым.

Итоговая проверка v0.782: прошли 70 целевых native/C89 тестов и семь тестов настоящего SDL-плеера с dummy-драйверами. Успешные пути полного ядра, включая передачу без разрешённых аллокаций, прошли ASan/UBSan; LeakSanitizer отключён из-за недоступного /proc. Внедрённые отказы аллокаций 0–4 прошли обычные проверки и UBSan изменённых модулей; раскрутка исключений под ASan не заявляется. Физические устройства и запуск на других платформах проверяются отдельно.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_mp3_probe` | function | [src/swf/sound_mp3.cpp:8](../src/swf/sound_mp3.cpp#L8) |
| `fvm_swf_sound_info` | function | [include/flashvm/swf_sound.h:134](../include/flashvm/swf_sound.h#L134) |
| `fvm_swf_sound_mixer_stop_id` | function | [include/flashvm/swf_sound_mixer.h:101](../include/flashvm/swf_sound_mixer.h#L101) |
| `fvm_swf_sound_mixer_submit_prepared` | function | [include/flashvm/swf_sound_mixer.h:69](../include/flashvm/swf_sound_mixer.h#L69) |
| `fvm_swf_sound_read_pcm` | function | [include/flashvm/swf_sound.h:148](../include/flashvm/swf_sound.h#L148) |
| `fvm_swf_stream_sound_snapshot_open_pcm` | function | [include/flashvm/swf_stream_sound.h:87](../include/flashvm/swf_stream_sound.h#L87) |
| `FVM_SWFMP3Decoder` | struct | [src/swf/sound_mp3.hpp:26](../src/swf/sound_mp3.hpp#L26) |
<!-- source-links:end -->
