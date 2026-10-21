# Последовательное PCM-чтение подготовленного потока

[English](STREAM_PCM_READER.md), [просмотр потоков](SWF_STREAM_SOUND.md).
`swf_stream_pcm.h` добавляет C89 курсор без выделения памяти поверх готового
неизменяемого [FVM_SWFStreamSoundSnapshot](../src/swf/sound_stream.cpp#L18). Это native звено для аудиопотоков
плеера и file/channel адаптеров, а не real-time воспроизведение потока.

## Владение и результат

Снимок готовит владелец VM через [`fvm_swf_stream_sound_snapshot_open`](../include/flashvm/swf_stream_sound.h#L65), затем
инициализирует собственный [FVM_SWFStreamPCMReader](../include/flashvm/swf_stream_pcm.h#L27). Снимок владеет копией encoded
байтов и индекса; reader заимствует его без allocation. Исходные context/runtime
можно уничтожить. Снимок должен жить до завершения всех читателей, после чего
его закрывают один раз. Деструктор reader не нужен. Один курсор используется
последовательно; независимые читатели могут делить неизменяемый снимок. State,
PCM buffer, metadata и счётчики не перекрываются. VM pointer/root, callback,
lock, часы или изменяемые глобальные данные не сохраняются.

next возвращает не более max_frames (>0), не пересекая encoded block. Capacity
и written считают отдельные S16 значения каналов, chunk.frames — sample frames
(стереопары). Metadata копирует stream/block index, номер movie frame от единицы,
first внутри блока, source rate/channels и флаги BEGIN, END, EMPTY. Сохраняются
переменные длины и пропуски movie frames. Явный пустой блок даёт OK, ноль значений
и BEGIN|END|EMPTY, продвигаясь на блок. Отсутствующий блок не создаёт тишину.
EOF даёт NOT_FOUND, сохраняя cursor и все выходы.

Сохраняется настоящая source PCM геометрия; playback advice не означает resampling.
PCM_NATIVE и PCM_LE используют существующий S16 decoder; сжатые MP3/ADPCM
сегменты дают UNSUPPORTED при init. Кодек, устройство, mixer, seek crossfade,
audio clock и A/V sync не добавляются. Немутое root stream воспроизведение
в desktop-плеере пока остаётся неподдерживаемым.

## Контракты API

| API | Контракт |
| --- | --- |
| [`fvm_swf_stream_pcm_reader_init`](../include/flashvm/swf_stream_pcm.h#L40) | Заимствует готовый PCM snapshot, копирует header/count и начинает с 0/0. INVALID_ARGUMENT при null, UNSUPPORTED при другом codec/short representation. Без allocation, ошибка сохраняет reader. |
| [`fvm_swf_stream_pcm_reader_next`](../include/flashvm/swf_stream_pcm.h#L48) | Читает ограниченный chunk, публикует metadata/written только при OK. Нужны точный chunk.struct_size, положительный max_frames, ёмкость в значениях каналов и PCM pointer для непустого результата. INVALID_ARGUMENT при неверных descriptor/capacity/range, NOT_FOUND на EOF. Ошибка сохраняет cursor, PCM, metadata и written. Успех всегда продвигается, включая пустые/end markers. |
| [`fvm_swf_stream_pcm_reader_seek`](../include/flashvm/swf_stream_pcm.h#L54) | Задаёт block и first <= sample_count; block_count/0 — явный EOF. Позиция на конце даёт нулевой END marker; EMPTY добавляется лишь для действительно пустого блока. Ошибка сохраняет cursor. Нет decoding/allocation/output side effects. |

Reader можно копировать как native снимок позиции с тем же требованием к
времени жизни source. Seek меняет только курсор: уже отправленный или сыгранный
PCM хост должен отменять/перематывать самостоятельно.

## Сборка и проверка

Модуль входит в core SDK, не требует SDL или аудиоустройства:

```sh
cmake -S flashVM -B build-stream-reader -DFVM_BUILD_TESTS=ON
cmake --build build-stream-reader --target flashvm_tests_swf_stream_pcm_reader \
  flashvm_tests_swf_stream_pcm_reader_api flashvm_tests_swf_stream_sound_snapshot \
  flashvm_tests_swf_stream_sound_snapshot_allocation
ctest --test-dir build-stream-reader -R 'swf_stream_(pcm_reader|sound_snapshot)' --output-on-failure
```

Raw SWF fixtures проверяют версии 3–8, native/little-endian PCM, mono/stereo,
8/16 бит, целочисленные эталоны, chunks, пустые блоки и пропуски кадров,
seek/end/EOF и атомарные ошибки буфера. Allocation-тест запрещает новые выделения
во время init/read/seek после уничтожения исходных context/runtime. Это не
закрывает sound-device/reference-player/platform приёмку.

## Исполняемый экспорт native курсором (v0.750)

[Прокомментированный swf_stream](../examples/multimedia/swf_stream/README.md)
добавляет `OUTPUT.pcm --cursor`. Он готовит поток встроенного SWF, уничтожает
context/runtime и использует только native reader и прямые C file callbacks.
Новая output VM/channel в этом режиме не создаётся. Пустой блок показывается,
но не вызывает write; source geometry сохраняется. Файл и metadata отчёт точно
совпадают с live и прежним `--prepared` экспортом. Несуществующий выходной каталог
даёт обычную ошибку с очисткой, без скрытого устройства.

Native OS-port тест запускает четыре настоящих worker после уничтожения VM/runtime.
Каждый владеет reader и делает 500 циклов rewind/read/EOF по одному неизменяемому
снимку, сравнивая каждое значение. Main присоединяет всех worker до закрытия
source. Это проверка Linux native threads, не foreign OS concurrency, слышимого
звука, real-time audio callback или A/V sync.

Проверка v0.750: прошли 72 выбранные native регрессионные проверки и 14 shared-host. Reader/snapshot пути и настоящий file tutorial прошли ASan/UBSan с независимым FFmpeg PCM эталоном; leak detection отключён ограничениями хоста, вся VM не инструментирована. Новый implementation и C89 consumer/tutorial objects собираются для ARM64 Linux/musl, RISC-V64 Linux/musl и Windows x64/MinGW; foreign execution или полная cross-сборка плеера этим не заявляются. Worker target требует включённого OS_PORTS. Все девять стадий открыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_pcm_reader_init` | function | [include/flashvm/swf_stream_pcm.h:40](../include/flashvm/swf_stream_pcm.h#L40) |
| `fvm_swf_stream_pcm_reader_next` | function | [include/flashvm/swf_stream_pcm.h:48](../include/flashvm/swf_stream_pcm.h#L48) |
| `fvm_swf_stream_pcm_reader_seek` | function | [include/flashvm/swf_stream_pcm.h:54](../include/flashvm/swf_stream_pcm.h#L54) |
| `fvm_swf_stream_sound_snapshot_open` | function | [include/flashvm/swf_stream_sound.h:65](../include/flashvm/swf_stream_sound.h#L65) |
| `FVM_SWFStreamPCMReader` | struct | [include/flashvm/swf_stream_pcm.h:27](../include/flashvm/swf_stream_pcm.h#L27) |
| `FVM_SWFStreamSoundSnapshot` | struct | [src/swf/sound_stream.cpp:18](../src/swf/sound_stream.cpp#L18) |
<!-- source-links:end -->
