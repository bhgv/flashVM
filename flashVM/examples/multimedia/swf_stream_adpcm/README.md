# Counted partial ADPCM / Неполные ADPCM-блоки с явными длинами

Executable C89 tutorial for native asset extraction, bounded decoding and worker
preparation. [EN contract](../../../doc/STREAM_ADPCM_COUNTED.md) /
[Контракт RU](../../../doc/STREAM_ADPCM_COUNTED.ru.md). Both modes destroy the
source context/runtime **before** reading/exporting PCM. No SDL, audio device,
FFmpeg, script compiler, media SDK or live MovieClip is required.

Исполняемый пример подходит для извлечения assets и подготовки native worker.
Длины каждого блока известны из построения fixture; average_samples=1 намеренно
не соответствует им. Это не эвристика определения длительности реального ролика.

## Input, ownership and output / Вход, владение и результат

[example_movie.h](example_movie.h) contains one actual FWS8 root ADPCM segment,
44100 Hz mono. Its three blocks hold five, zero and three frames. The first block
uses four-bit codes, predictor 100/index 0 and codes 7/15/3/11. The third uses
three-bit codes, predictor -100/index 12 and codes 3/7. Each compressed block
starts its own width/predictor/index; the middle empty block retains its ordinal.
No PCM is synthesized for that movie frame.

The required output is exactly eight native-S16 values (16 bytes):
`100, 111, 81, 110, 85, -100, -61, -120`. File byte order follows the host; this is
raw PCM, not WAV. The four-frame caller buffer is independent of source ownership.
Partial/failed file writes are terminal; an output file can remain partial after
failure. No atomic queue/retry guarantee is invented for stdio.

| Mode | Native ownership and work |
| --- | --- |
| OUTPUT.pcm | Prepare normalized PCM under a 64 KiB peak snapshot ceiling, free VM, copy chunks with a native PCM reader |
| OUTPUT.pcm --ranges | Copy bounded encoded ADPCM, free VM, validate/decode each selected range without further native allocation |

The peak ceiling covers the snapshot object/index/payloads, not the whole process,
count table, VM, file buffering or caller stack. Range mode uses less retained
sample storage but revalidates/decodes from block start on every read; it is for
inspection/extraction, not a low-latency playback loop. The host retains the
snapshot until reading and file output stop, then closes it once. Cleanup covers
partial runtime/context/preparation acquisition and file errors.

В обоих режимах массив длин заимствуется только на время вызова. Нормализованный
snapshot владеет PCM, encoded snapshot — исходными сжатыми байтами. В export loop
нет исходной VM, managed references или device callbacks. Перед close должны
завершиться все читатели; пример однопоточный и не запускает скрытого worker.

## Build and run / Сборка и запуск

From repository root / из корня репозитория:

```sh
cmake -S flashVM -B build-adpcm \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON
cmake --build build-adpcm --target flashvm_examples_multimedia_swf_stream_adpcm_main
./build-adpcm/flashvm_examples_multimedia_swf_stream_adpcm_main
./build-adpcm/flashvm_examples_multimedia_swf_stream_adpcm_main decoded.pcm
./build-adpcm/flashvm_examples_multimedia_swf_stream_adpcm_main ranged.pcm --ranges
python3 flashVM/scripts/check_swf_stream_adpcm_example.py \
  ./build-adpcm/flashvm_examples_multimedia_swf_stream_adpcm_main
ctest --test-dir build-adpcm --output-on-failure -R 'swf_stream_adpcm_(main|tutorial)'
```

A C89/C++98 toolchain and CMake are sufficient; the minimal profile uses bundled
static core dependencies. NO_MEDIA does not disable the native SWF decoder.
No arguments prints usage without opening a file/VM. Invalid arguments fail before
startup. Both successful exports report `Decoded 8 counted ADPCM frames after VM teardown.`
The Python checker verifies every hand-calculated sample, replacement of an old
file, usage, unknown options and missing-directory failure in both modes.

## Demonstrated APIs / Использованные API

Each function below is used in [main.c](main.c); detailed count/range semantics:
[EN](../../../doc/STREAM_ADPCM_COUNTED.md) / [RU](../../../doc/STREAM_ADPCM_COUNTED.ru.md).

| Function | Purpose, ownership and execution |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L62) | Initialize caller configuration synchronously; no runtime owner yet |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L64) | Allocate the source runtime on owner; release after native preparation |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L68) | Allocate a source context borrowing runtime; destroy before runtime |
| [fvm_load_memory](../../../include/flashvm/flashvm.h#L72) | Validate/copy the literal SWF into source context; may allocate, no playback loop |
| [fvm_swf_stream_sound_snapshot_open_adpcm_counted](../../../include/flashvm/swf_stream_sound.h#L111) | Decode exact per-block lengths under peak ceiling; publish independent immutable PCM, allocate on owner |
| [fvm_swf_stream_sound_snapshot_open_bounded](../../../include/flashvm/swf_stream_sound.h#L72) | Copy encoded root ADPCM/index under peak ceiling without codec decoding; retain no VM pointer |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) | Release managed source after native ownership succeeds; no native snapshot invalidation |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | End source runtime after its context; export continues with native storage only |
| [fvm_swf_stream_pcm_reader_init](../../../include/flashvm/swf_stream_pcm.h#L40) | Initialize caller cursor borrowing normalized snapshot; no allocation/callback/VM |
| [fvm_swf_stream_pcm_reader_next](../../../include/flashvm/swf_stream_pcm.h#L48) | Copy bounded native PCM and exact chunk metadata; written counts values, NOT_FOUND is EOF |
| [fvm_swf_stream_sound_snapshot_read_adpcm](../../../include/flashvm/swf_stream_sound.h#L150) | Validate full encoded block, decode explicit range into caller buffer; no allocation/callback, repeated block-length work |
| [fvm_swf_stream_sound_snapshot_close](../../../include/flashvm/swf_stream_sound.h#L154) | Null-safe release after every reader/range call ends; no device close or source VM required |

## Limits / Ограничения

Counts are an explicit fixture/host assertion. Padding may fit several counts;
passing geometry checks does not prove original movie timing. Automatic partial
stream playback, root event mixing policy, child streams, A/V synchronization,
MP3 and physical-device/foreign-platform acceptance remain open. This tutorial
adds no timeline clock, thread scheduler or new JIT behavior. All roadmap stages
remain open. / Пример не закрывает полную SWF≤8 совместимость или приёмку устройств.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../../../include/flashvm/flashvm.h#L72) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_swf_stream_pcm_reader_init` | function | [include/flashvm/swf_stream_pcm.h:40](../../../include/flashvm/swf_stream_pcm.h#L40) |
| `fvm_swf_stream_pcm_reader_next` | function | [include/flashvm/swf_stream_pcm.h:48](../../../include/flashvm/swf_stream_pcm.h#L48) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:154](../../../include/flashvm/swf_stream_sound.h#L154) |
| `fvm_swf_stream_sound_snapshot_open_adpcm_counted` | function | [include/flashvm/swf_stream_sound.h:111](../../../include/flashvm/swf_stream_sound.h#L111) |
| `fvm_swf_stream_sound_snapshot_open_bounded` | function | [include/flashvm/swf_stream_sound.h:72](../../../include/flashvm/swf_stream_sound.h#L72) |
| `fvm_swf_stream_sound_snapshot_read_adpcm` | function | [include/flashvm/swf_stream_sound.h:150](../../../include/flashvm/swf_stream_sound.h#L150) |
<!-- source-links:end -->
