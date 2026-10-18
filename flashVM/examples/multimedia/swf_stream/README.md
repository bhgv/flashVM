# swf_stream — frame-addressed streaming PCM into a native file bridge

Sources: [main.c](main.c), [example_constants.h](example_constants.h).

## Назначение

Исполняемый C89 пример перечисляет корневой streaming-sound segment и его
SoundStreamBlock из встроенного FWS4. Исходный PCM — stereo 44100 Hz U8;
playback recommendation намеренно mono 11025 Hz. Пример использует source
geometry, показывает variable-length blocks и пропущенный movie frame.
Это явный экспорт, без real-time воспроизведения, устройства или скрытого clock.

Каждая непустая строка source/header подписана; ошибок через assert нет.
Host bridge вызывает настоящие fopen/fwrite/fclose, его можно заменить своим
системным выходом с тем же [FVM_MediaOutput](../../../include/flashvm/multimedia.h#L67) ABI. Мост синхронно заимствует PCM.

## Подготовка и запуск

Из корня проекта:

```sh
cmake -S . -B build-stream -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-stream --target flashvm_examples_multimedia_swf_stream_main --parallel
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm
```

Нужны C89/C++98 toolchain и writable output path. NO_MEDIA/NO_GRAPHICS достаточны:
FFmpeg, ALSA, модель распознавания, permissions и реальные audio devices не нужны.
Названный файл может быть перезаписан. Без аргументов только usage, без VM/файлов.
На Windows/macOS/Android замените путь/запуск согласно native toolchain; Linux
проверка не заявляется native acceptance этих ОС. Встроенный input не требует
загрузки внешнего SWF или установки Flash Player.

## Результат и реализация

```text
Frame 1: 2 sample frames
Frame 3: 1 sample frames
Frame 4: 0 sample frames
Exported 3 sample frames from 3 blocks.
```

Файл — 12 bytes native-endian S16 stereo:
`[-32768, 32512, 0, 0, 32512, -32768]`. Frame 2 отсутствует и не заполняется
тишиной. Среднее в заголовке равно 1, но первый block содержит 2 sample frames;
последний empty block учитывается в metadata и не вызывает output write.
Буфер содержит всего один stereo frame, первый блок читается двумя ranges.

[GC:] Context owns canonical SWF bytes и регистрацию output. Header — copied
scalars; block data — borrowed до успешного reload/context_destroy. Пример
никогда не хранит payload pointer для callback: read_pcm сначала копирует в
caller short buffer, затем channel_write синхронно вызывает FILE bridge.
В callbacks этого примера нет reload/GC; если вы их добавите, заново проверяйте
ordinals/geometry после возврата — старый block view может стать невалидным.

FILE/userdata принадлежат host и живут до конца context_destroy. Stop-all
закрывает actual output; terminal cleanup повторяет остановку ещё active канала.
При failed write FILE может остаться вне active state: fallback fclose после
context teardown освобождает такой handle. Runtime освобождается последним.
Ошибки не вызывают неявного повтора уже доставленных bytes.

Для своего системного моста замените write_pcm/stop_pcm; соблюдайте синхронное
заимствование, owning VM thread, независимое владение устройством и запрет
GC/registry mutation внутри callback. См. [MULTIMEDIA_BRIDGES.md](../../../doc/MULTIMEDIA_BRIDGES.md)
и [ALSA_AUDIO_OUTPUT.md](../../../doc/ALSA_AUDIO_OUTPUT.md). Host сам реализует
реальное время, gap silence, rate/channel conversion, смену segment и seek.
Этот пример не декодирует compressed stream и не исполняет MovieClip timeline.

## Демонстрируемые API

- [`fvm_swf_stream_sound_count`](../../../include/flashvm/swf_stream_sound.h#L39), [`fvm_swf_stream_sound_at`](../../../include/flashvm/swf_stream_sound.h#L41) — сегменты и copied header:
  [SWF_STREAM_SOUND.md](../../../doc/SWF_STREAM_SOUND.md#fvm_swf_stream_sound_count),
  [header query](../../../doc/SWF_STREAM_SOUND.md#fvm_swf_stream_sound_at).
- [`fvm_swf_stream_sound_block_count`](../../../include/flashvm/swf_stream_sound.h#L43), [`fvm_swf_stream_sound_block_at`](../../../include/flashvm/swf_stream_sound.h#L45) — actual frame,
  empty/missing blocks и borrowed payload:
  [block count](../../../doc/SWF_STREAM_SOUND.md#fvm_swf_stream_sound_block_count),
  [block view](../../../doc/SWF_STREAM_SOUND.md#fvm_swf_stream_sound_block_at).
- [`fvm_swf_stream_sound_read_pcm`](../../../include/flashvm/swf_stream_sound.h#L47) — bounded copies без callbacks/allocations:
  [range contract](../../../doc/SWF_STREAM_SOUND.md#fvm_swf_stream_sound_read_pcm).
- [`fvm_audio_channel_create`](../../../include/flashvm/multimedia.h#L81), [`fvm_audio_channel_write`](../../../include/flashvm/multimedia.h#L83), [`fvm_audio_stop_all`](../../../include/flashvm/multimedia.h#L91) —
  copied output table, actual native writes, active state and stop:
  [AUDIO_CHANNELS.md](../../../doc/AUDIO_CHANNELS.md).
- [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62), [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64), [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68), [`fvm_load_memory`](../../../include/flashvm/flashvm.h#L72),
  [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70), [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) — dependency-ordered VM ownership:
  [API_INDEX.md](../../../doc/API_INDEX.md#runtime-and-context),
  [canonical input](../../../doc/SWF_SOUND_RESOURCES.md#scope--область-реализации).

## English

This line-commented C89 tutorial reads a literal FWS4 stream with unequal stereo
PCM blocks, a missing movie frame and an explicit empty block. Advisory playback
geometry differs from source geometry; only source rate/channels drive output.
It reads one sample frame at a time into caller shorts, then submits through
real synchronous stdio callbacks. Output is exactly three stereo frames (12 bytes)
with the values shown above. No timing/silence insertion or audible playback is
implied. NO_MEDIA/NO_GRAPHICS and an explicit writable output path suffice.

The context owns canonical bytes and channel registration; the host owns FILE,
userdata and caller buffers. Borrowed block data must not survive reload; decoded
shorts do. Stop-all flushes/closes actual output, context cleanup runs while
userdata is live, and fallback fclose handles failed-write ownership before
runtime destruction. No arguments create no files/VM; native errors use normal
cleanup without assertions or implicit replay. Native device adapters, timing,
compressed codecs and foreign-platform acceptance are separate concerns.

## Validation / Проверка исполняемого примера

```sh
python3 scripts/check_swf_stream_example.py build-stream/flashvm_examples_multimedia_swf_stream_main
python3 scripts/check_swf_stream_example.py build-stream/flashvm_examples_multimedia_swf_stream_main --ffmpeg
```

Первая проверка запускает настоящий бинарник, сверяет frame metadata и все bytes,
проверяет замену файла, отсутствие side effects без аргументов и cleanup при
ошибке native path. Она включена в native CTest как
`flashvm_swf_stream_tutorial_host`; cross builds не запускают целевой binary на
host. Второй вариант дополнительно требует системный ffmpeg и сверяет raw U8
→ native S16 stereo conversion. Это независимый PCM oracle, а не доказательство
полного SWF demux/stream sync в FFmpeg и не runtime dependency FlashVM.

The native CTest harness exercises the real executable, exact metadata/PCM,
replacement and failure cleanup. Optional --ffmpeg checks independently known
raw payload conversion; it is neither a runtime dependency nor a complete SWF
playback oracle. Cross builds omit this host-side process harness.

## Prepared mode / Подготовленный режим

Pass `--prepared` after the output path:

```sh
./build-jit/flashvm_examples_multimedia_swf_stream_main output.pcm --prepared
```

This mode explicitly prepares an immutable indexed PCM snapshot, destroys the
original context and runtime, then creates a new **unloaded** VM only for the
ordinary file-output registration. Header queries and bounded PCM reads therefore
come entirely from the independent snapshot. Output and movie-frame diagnostics
are identical to the default live mode. Preparation can fail with OUT_OF_MEMORY;
cleanup releases the snapshot independently of output-VM ownership. No additional
system package is required. It trades one payload/index copy for constant-time
block lookup instead of repeatedly scanning all root tags.

Режим явно копирует PCM и индекс, уничтожает исходную VM и создаёт новую,
**без загруженного SWF**, только для файлового переходника. Метаданные и PCM
берутся из независимого снимка; вывод полностью совпадает с обычным режимом.
Подготовка требует памяти, очистка снимка независима от очистки новой VM.

Additional demonstrated API (contracts, limits and lifetime in
[SWF_STREAM_SOUND.md](../../../doc/SWF_STREAM_SOUND.md#prepared-immutable-pcm-index--подготовленный-неизменяемый-индекс-pcm)):

- [`fvm_swf_stream_sound_snapshot_open`](../../../include/flashvm/swf_stream_sound.h#L51): validate, budget and copy the selected segment.
- [`fvm_swf_stream_sound_snapshot_info`](../../../include/flashvm/swf_stream_sound.h#L53): inspect the copied source/playback header.
- [`fvm_swf_stream_sound_snapshot_block_count`](../../../include/flashvm/swf_stream_sound.h#L55): enumerate the prepared index.
- [`fvm_swf_stream_sound_snapshot_block_at`](../../../include/flashvm/swf_stream_sound.h#L57): borrow original-wire bytes until close.
- [`fvm_swf_stream_sound_snapshot_read_pcm`](../../../include/flashvm/swf_stream_sound.h#L59): convert bounded independent ranges.
- [`fvm_swf_stream_sound_snapshot_close`](../../../include/flashvm/swf_stream_sound.h#L61): release native storage even after VM teardown.

The host checker runs both modes, compares exact stdout/native-S16 bytes and checks
actual missing-directory write failure cleanup in both modes. This is file bridge
acceptance, not physical audio-device or automatic MovieClip clock acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:81](../../../include/flashvm/multimedia.h#L81) |
| `fvm_audio_channel_write` | function | [include/flashvm/multimedia.h:83](../../../include/flashvm/multimedia.h#L83) |
| `fvm_audio_stop_all` | function | [include/flashvm/multimedia.h:91](../../../include/flashvm/multimedia.h#L91) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../../../include/flashvm/flashvm.h#L72) |
| `FVM_MediaOutput` | struct | [include/flashvm/multimedia.h:67](../../../include/flashvm/multimedia.h#L67) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_swf_stream_sound_at` | function | [include/flashvm/swf_stream_sound.h:41](../../../include/flashvm/swf_stream_sound.h#L41) |
| `fvm_swf_stream_sound_block_at` | function | [include/flashvm/swf_stream_sound.h:45](../../../include/flashvm/swf_stream_sound.h#L45) |
| `fvm_swf_stream_sound_block_count` | function | [include/flashvm/swf_stream_sound.h:43](../../../include/flashvm/swf_stream_sound.h#L43) |
| `fvm_swf_stream_sound_count` | function | [include/flashvm/swf_stream_sound.h:39](../../../include/flashvm/swf_stream_sound.h#L39) |
| `fvm_swf_stream_sound_read_pcm` | function | [include/flashvm/swf_stream_sound.h:47](../../../include/flashvm/swf_stream_sound.h#L47) |
| `fvm_swf_stream_sound_snapshot_block_at` | function | [include/flashvm/swf_stream_sound.h:57](../../../include/flashvm/swf_stream_sound.h#L57) |
| `fvm_swf_stream_sound_snapshot_block_count` | function | [include/flashvm/swf_stream_sound.h:55](../../../include/flashvm/swf_stream_sound.h#L55) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:61](../../../include/flashvm/swf_stream_sound.h#L61) |
| `fvm_swf_stream_sound_snapshot_info` | function | [include/flashvm/swf_stream_sound.h:53](../../../include/flashvm/swf_stream_sound.h#L53) |
| `fvm_swf_stream_sound_snapshot_open` | function | [include/flashvm/swf_stream_sound.h:51](../../../include/flashvm/swf_stream_sound.h#L51) |
| `fvm_swf_stream_sound_snapshot_read_pcm` | function | [include/flashvm/swf_stream_sound.h:59](../../../include/flashvm/swf_stream_sound.h#L59) |
<!-- source-links:end -->
