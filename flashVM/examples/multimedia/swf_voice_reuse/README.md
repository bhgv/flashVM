# Native voice replay / Повторное использование голоса

Executable strict-C89 tutorial: decode the first root StartSound once, destroy its
context/runtime, retain a completed native template, deep-clone it twice, restart
each copy and transfer it into a bounded mixer. Export exactly two complete
replays, including authored intervals, repetitions and envelopes. Supports the
existing PCM8/PCM16, ADPCM and supported MP3 voice decoders; stop events are not
playable templates. No audio device, SDL, timeline, network or callback is needed.

Исполняемый пример C89: первый StartSound декодируется один раз, VM уничтожается,
завершённый нативный шаблон дважды копируется, каждая копия перезапускается и
передаётся микшеру с бюджетом. В файл записываются ровно два полных воспроизведения,
с учётом диапазона, повторов и огибающей. Поддерживаются существующие декодеры
PCM8/PCM16, ADPCM и MP3; stop-событие шаблоном не является. NO_MEDIA достаточен.

## Build and run / Сборка и запуск

```sh
cmake -S flashVM -B build-reuse \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON
cmake --build build-reuse --target flashvm_examples_multimedia_swf_voice_reuse_main
./build-reuse/flashvm_examples_multimedia_swf_voice_reuse_main
./build-reuse/flashvm_examples_multimedia_swf_voice_reuse_main input.swf replay.pcm
python3 flashVM/scripts/check_swf_voice_reuse_example.py ./build-reuse/flashvm_examples_multimedia_swf_voice_reuse_main
```

Requires C89/C++98, CMake and the component's bundled static dependencies. Output
is native-endian interleaved S16 with the printed effective rate/channel count,
without a WAV header. Its two halves must be identical. Source file is limited
to 16 MiB; runtime payload and aggregate native template/mixer/incoming clone
charges are limited to 32 MiB each; complete export to 32 million frames. Runtime
and native charges are separate. The input copy, transfer stack, allocator
bookkeeping and stdio buffers are outside the native-owner ceiling. A long source
can fit the runtime yet fail the combined template/clone budget; that is expected.

Выход — native-endian S16 с напечатанными частотой/числом каналов, без WAV-заголовка;
две половины одинаковы. Вход ограничен 16 MiB, runtime payload и сумма нативных
шаблона/микшера/входящей копии — по 32 MiB, весь экспорт — 32 миллионами кадров.
Это отдельные бюджеты. Входной буфер, стек передачи, накладные расходы аллокатора
и stdio в бюджет нативных владельцев не входят. Вмещающийся в runtime источник
может не пройти общий бюджет двух копий — это ожидаемый явный отказ.

## Ownership and errors / Владение и ошибки

All source preparation is owner-thread work. VM destruction precedes clone,
seek, mixer creation and output-file open. A template at EOF stays unchanged;
cloning preserves that EOF, and seek(0,0) restarts only the independent copy.
Successful bounded adoption clears the copy handle. Each finite read is limited
to the authored duration, preventing infinite mixer silence. The template stays
alive through cloning; the mixer owns adopted copies through their final reads.
Cleanup closes any untransferred copy, mixer, then template. No ownership is
shared between replays. Zero-duration input writes an empty file successfully.

Подготовка выполняется на владельце контекста. Уничтожение VM предшествует clone,
seek, созданию микшера и открытию выхода. Шаблон остаётся на EOF; копия наследует
EOF и отдельно перезапускается seek(0,0). Успешная передача обнуляет её указатель.
Чтение ограничено авторской длительностью, без бесконечного хвоста тишины.
Cleanup закрывает непереданную копию, микшер и затем шаблон. Нулевая длительность
успешно создаёт пустой выход.

No arguments prints usage without side effects. Invalid CLI, missing/oversized
input, malformed SWF, unsupported codec, budget and native file failures return
nonzero. Output is opened/replaced only after first successful adoption. Later
allocation/write/flush errors may leave a partial file; stdio is not an atomic
queue retry. This example is not a real-time player, movie seek or a general
cache with shared PCM/refcounts; every clone consumes its own complete arrays.

Без аргументов выводится справка без побочных эффектов. Ошибки CLI, входа, SWF,
кодека, бюджета и файлов возвращают ненулевой код. Выход заменяется только после
первой успешной передачи; поздние ошибки могут оставить частичный файл. Это не
атомарная очередь, real-time плеер, перемотка клипа или общий кэш PCM.

## APIs in main.c / API и контракты

New native contracts: [storage](../../../doc/NATIVE_VOICES.md#v0783-exact-storage-and-preparation-ceilings),
[cloning](../../../doc/NATIVE_VOICES.md#v0784-reusable-prepared-pcm),
[positioning](../../../doc/NATIVE_VOICES.md#v0785-native-cursor-positioning),
[adoption](../../../doc/NATIVE_VOICES.md#v0786-aggregate-mixer-ownership),
[construction](../../../doc/NATIVE_VOICES.md#v0789-reject-fixed-mixer-storage-before-allocation).
Russian contracts: [NATIVE_VOICES.ru.md](../../../doc/NATIVE_VOICES.ru.md).
Existing runtime and mixer contracts: [runtime declarations](../../../include/flashvm/flashvm.h),
[SWF_SOUND_MIXER.md](../../../doc/SWF_SOUND_MIXER.md).

| Function | Demonstrated operation and lifetime |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L70) | Initialize every runtime-policy field before changing the source payload limit |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L74) | Acquire source runtime; context must be destroyed before runtime |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L82) | Acquire owner-thread source context borrowing runtime |
| [fvm_load_memory](../../../include/flashvm/flashvm.h#L90) | Validate/copy SWF; input malloc storage is released on success |
| [fvm_swf_sound_voice_open_bounded](../../../include/flashvm/swf_sound.h#L119) | Decode first event once under native object/PCM/envelope ceiling; null on error |
| [fvm_swf_sound_voice_info](../../../include/flashvm/swf_sound.h#L142) | Copy effective geometry and finite duration without a callback |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L86) | Release managed source before every replay operation |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L78) | End source runtime before native cloning/output |
| [fvm_swf_sound_voice_storage_info](../../../include/flashvm/swf_sound.h#L125) | Observe exact template storage to reserve its retained charge |
| [fvm_swf_sound_mixer_create_bounded](../../../include/flashvm/swf_sound_mixer.h#L78) | Allocate configured mixer only when its fixed storage fits remaining bytes |
| [fvm_swf_sound_mixer_storage_info](../../../include/flashvm/swf_sound_mixer.h#L135) | Subtract fixed mixer storage before preparing a clone |
| [fvm_swf_sound_voice_seek](../../../include/flashvm/swf_sound.h#L153) | Keep template at EOF and restart each independently owned copy without decode/allocation |
| [fvm_swf_sound_voice_clone](../../../include/flashvm/swf_sound.h#L137) | Allocate independent current-progress PCM/envelope owner within remaining bytes; no VM |
| [fvm_swf_sound_mixer_submit_prepared_bounded](../../../include/flashvm/swf_sound_mixer.h#L119) | Recheck aggregate mixer/clone bytes, consume and clear copy only on success |
| [fvm_swf_sound_mixer_read](../../../include/flashvm/swf_sound_mixer.h#L157) | Read only finite authorized frames; scalar capacity/written count channel values |
| [fvm_swf_sound_voice_close](../../../include/flashvm/swf_sound.h#L162) | Release untransferred copy or retained template after all borrowers stop |
| [fvm_swf_sound_mixer_close](../../../include/flashvm/swf_sound_mixer.h#L184) | Release adopted owners after every native read is quiescent |

The checker uses independent FWS/CWS PCM/range/loop/envelope and ADPCM vectors,
all six MP3 geometries with optional full FFmpeg PCM, exact lengths, identical
replays, empty input, output replacement and terminal-error side effects. It is
host/sample evidence, not device, foreign-platform or complete Flash acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:90](../../../include/flashvm/flashvm.h#L90) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_swf_sound_mixer_close` | function | [include/flashvm/swf_sound_mixer.h:184](../../../include/flashvm/swf_sound_mixer.h#L184) |
| `fvm_swf_sound_mixer_create_bounded` | function | [include/flashvm/swf_sound_mixer.h:78](../../../include/flashvm/swf_sound_mixer.h#L78) |
| `fvm_swf_sound_mixer_read` | function | [include/flashvm/swf_sound_mixer.h:157](../../../include/flashvm/swf_sound_mixer.h#L157) |
| `fvm_swf_sound_mixer_storage_info` | function | [include/flashvm/swf_sound_mixer.h:135](../../../include/flashvm/swf_sound_mixer.h#L135) |
| `fvm_swf_sound_mixer_submit_prepared_bounded` | function | [include/flashvm/swf_sound_mixer.h:119](../../../include/flashvm/swf_sound_mixer.h#L119) |
| `fvm_swf_sound_voice_clone` | function | [include/flashvm/swf_sound.h:137](../../../include/flashvm/swf_sound.h#L137) |
| `fvm_swf_sound_voice_close` | function | [include/flashvm/swf_sound.h:162](../../../include/flashvm/swf_sound.h#L162) |
| `fvm_swf_sound_voice_info` | function | [include/flashvm/swf_sound.h:142](../../../include/flashvm/swf_sound.h#L142) |
| `fvm_swf_sound_voice_open_bounded` | function | [include/flashvm/swf_sound.h:119](../../../include/flashvm/swf_sound.h#L119) |
| `fvm_swf_sound_voice_seek` | function | [include/flashvm/swf_sound.h:153](../../../include/flashvm/swf_sound.h#L153) |
| `fvm_swf_sound_voice_storage_info` | function | [include/flashvm/swf_sound.h:125](../../../include/flashvm/swf_sound.h#L125) |
<!-- source-links:end -->
