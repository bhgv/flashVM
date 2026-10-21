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

[fvm_swf_sound_read_pcm](../include/flashvm/swf_sound.h#L108) теперь читает MP3 в SWF4–8 без allocation/callback. Диапазон
отсчитывается после неотрицательного SI16 SeekSamples. Сначала проверяются все
полные packets; затем новый stack decoder копирует только выбранные кадры.
Объявленная длина должна помещаться после latency; полный encoder tail проверяется,
но не выдаётся сверх sample_count. Скрытой коррекции LAME/Xing gapless нет.
Даже нулевое чтение проверяет весь источник. Изменение/teardown и output buffers
сериализуются owner; managed pointers после возврата не остаются.

[fvm_swf_sound_info](../include/flashvm/swf_sound.h#L94) выдаёт фактические MPEG rate/channels из первого корректного
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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_mp3_probe` | function | [src/swf/sound_mp3.cpp:8](../src/swf/sound_mp3.cpp#L8) |
| `fvm_swf_sound_info` | function | [include/flashvm/swf_sound.h:94](../include/flashvm/swf_sound.h#L94) |
| `fvm_swf_sound_read_pcm` | function | [include/flashvm/swf_sound.h:108](../include/flashvm/swf_sound.h#L108) |
| `FVM_SWFMP3Decoder` | struct | [src/swf/sound_mp3.hpp:26](../src/swf/sound_mp3.hpp#L26) |
<!-- source-links:end -->
