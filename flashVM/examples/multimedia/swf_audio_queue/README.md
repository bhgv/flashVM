# Replaceable PCM queue / Сменная очередь PCM

## Purpose / Назначение

Исполняемый C89 учебник демонстрирует контракт собственного моста аудиовывода:
фиксированный pending-буфер, повтор отвергнутой записи, отдельные pause/clear,
часы потребления и порядок освобождения. `--demo` использует детерминированную
модель FIFO; `--sdl` явно открывает настоящий SDL playback device. Пустой микшер
выдаёт тишину: пример изучает протокол, не слышимость и не воспроизведение фильма.
Для реальных StartSound/PCM/ADPCM и timeline см. [swf_event_mixer](../swf_event_mixer/README.md).

Runnable C89 tutorial of a custom queue callback table and optional native SDL
adapter. The custom model discards silence values and tracks frames only; it is
not a production audio bridge. Real bridges must copy complete interleaved PCM
before successful return. No VM, SWF, filesystem writes or microphone is needed.

## Build and run / Сборка и запуск

```sh
cmake -S flashVM -B /tmp/flashvm-build -DFVM_BUILD_EXAMPLES=ON
cmake --build /tmp/flashvm-build --target flashvm_examples_multimedia_swf_audio_queue_main
/tmp/flashvm-build/flashvm_examples_multimedia_swf_audio_queue_main --demo
```

Expected exact model output / ожидаемый вывод модели:

```text
accepted=128 queued=64 played=64
```

The first write is rejected without acceptance. Pump retries the same 128-frame
buffer; resume then simulated half-block consumption produces the copied clock.
Pause on create, one-block pumping, observation and explicit stop are visible in
[main.c](main.c); every nonempty code line has a tutorial comment, no asserts.
Без аргументов — usage/exit 0; неверные аргументы — exit 2; API failures — exit 1;
успех — exit 0. `--sdl` в обычной сборке возвращает [`FVM_UNSUPPORTED`](../../../include/flashvm/flashvm.h#L46)/exit 1,
не подменяя устройство моделью.

<a id="native-preparation"></a>
## Native preparation / Подготовка ОС

Enable the optional installed SDL2 >=2.0.9 CMake CONFIG package:

```sh
cmake -S flashVM -B /tmp/flashvm-audio -DFVM_BUILD_EXAMPLES=ON -DFVM_ENABLE_SDL_AUDIO=ON
cmake --build /tmp/flashvm-audio --target flashvm_examples_multimedia_swf_audio_queue_main
/tmp/flashvm-audio/flashvm_examples_multimedia_swf_audio_queue_main --sdl
```

- Linux: install your distribution's SDL2 development SDK (e.g. libsdl2-dev),
  run inside a session with an available playback service/device and its access
  permissions. Headless servers may have no audio endpoint. SDL chooses the
  installed driver; no fallback driver is selected by this example.
- Windows: use an SDL2 SDK matching compiler/architecture, set SDL2_DIR to its
  CMake package if necessary, and put its SDL2.dll beside the executable or on
  PATH. Build/run with the platform toolchain; the bridge uses SDL_SetMainReady
  and does not require SDL_main ownership. A working default output is needed.
- macOS: install a matching SDL2 SDK/CMake config, set SDL2_DIR when nonstandard,
  provide the linked dylib/framework according to its installation and run in a
  logged-in audio session. Integrate any additional window event loop on the
  required main thread in a full application; this tutorial opens audio only.
- Android: this standalone desktop command is not an Android activity/JNI bridge.
  Android lifecycle, SDK packaging and native output acceptance remain open.

Нужны playback endpoint/служба и соответствующий SDK. Разрешения микрофона не
нужны: захвата нет. Не вызывайте SDL_Quit, пока устройство живо. Закрытие producer
раньше device, затем SDL_CloseAudioDevice и баланс SDL_InitSubSystem выполняются
соответствующими владельцами. Не передавайте управление SDL lifecycle независимому
frontend без согласования владения.

`--sdl` primes/resumes/queries/stops one tiny silence block immediately. Its counts
are asynchronous; do not compare them to model stdout or treat silence as audible
hardware evidence. Optional `FVM_SDL_AUDIO_DEVICE_TESTS=ON` registers a separate
real-SDK test with SDL_AUDIODRIVER=dummy that waits for FIFO depletion. This
checks queue/pause/clock protocol only. Native Linux hardware listening,
Windows/macOS and sustained playback acceptance remain separate work.

## Implementation / Реализация

The backend table is copied; stack userdata stays live until producer close.
The mixer, producer and native device have independent ownership. Error cleanup
closes producer, mixer, then device, including when stop fails. Close alone does
not silence an already queued device; stop performs pause/clear first.
On clear failure the producer stays paused with pending data and voices retained.
Do not keep the producer-owned PCM pointer in a callback or report an error after
partial acceptance. A production adapter needs its own copy/transaction buffer
if its system API only supports partial writes. Serialize the control thread;
backend callbacks must not recursively operate on output or its borrowed mixer.

Это не realtime callback: producer выполняется host-потоком, SDL потребляет свою
копию параллельно. `played_frames` означает извлечение из программной очереди,
не позицию ЦАП. При underrun часы замирают; video catch-up здесь не реализован.
Микшер без голосов позволяет отдельно изучить контракт. Подключение реальных
событий использует mixer_submit из соседнего учебника, без изменения queue ABI.

## Demonstrated API / Демонстрируемые функции

Full contracts: [SWF_SOUND_OUTPUT.md](../../../doc/SWF_SOUND_OUTPUT.md),
[RU](../../../doc/SWF_SOUND_OUTPUT.ru.md); source declarations are linked below.
Every operation runs on the serialized host thread; no API owns a borrowed mixer.

| API | Individual behavior and guide |
| --- | --- |
| [`fvm_swf_sound_output_create`](../../../include/flashvm/swf_sound_output.h#L50) | Allocates fixed pending PCM, borrows mixer/backend, pauses and requires empty FIFO; synchronous. [Creation](../../../doc/SWF_SOUND_OUTPUT.md#creation). |
| [`fvm_swf_sound_output_pump`](../../../include/flashvm/swf_sound_output.h#L124) | Prepares/retries at most one complete block, retains PCM after rejected write; invokes synchronous native callbacks. [Pump](../../../doc/SWF_SOUND_OUTPUT.md#pump-and-retry). |
| [`fvm_swf_sound_output_pause`](../../../include/flashvm/swf_sound_output.h#L146) | Sets consumption without discarding or implicitly priming; state changes only on callback success. [Pause](../../../doc/SWF_SOUND_OUTPUT.md#pause-and-copied-clock). |
| [`fvm_swf_sound_output_info`](../../../include/flashvm/swf_sound_output.h#L148) | Queries FIFO and copies consumed-frame time; preserves caller output on error, no borrowed result. [Clock fields](../../../doc/SWF_SOUND_OUTPUT.md#pause-and-copied-clock). |
| [`fvm_swf_sound_output_stop`](../../../include/flashvm/swf_sound_output.h#L152) | Pauses/clears, stops all voices and resets epoch; clear error leaves paused recoverable state. [Stop](../../../doc/SWF_SOUND_OUTPUT.md#stop-and-close). |
| [`fvm_swf_sound_output_close`](../../../include/flashvm/swf_sound_output.h#L163) | Frees pending/producer only, no callbacks/device/mixer ownership; null allowed. [Close](../../../doc/SWF_SOUND_OUTPUT.md#stop-and-close). |
| [`fvm_audio_sdl_queue_open`](../../../include/flashvm/swf_sound_output.h#L170) | Synchronously acquires a real paused playback device and callback table; may fail/UNSUPPORTED. [SDL](../../../doc/SWF_SOUND_OUTPUT.md#sdl2-native-bridge). |
| [`fvm_audio_sdl_queue_close`](../../../include/flashvm/swf_sound_output.h#L186) | Releases device and subsystem reference after output close; null succeeds, no VM access. [SDL cleanup](../../../doc/SWF_SOUND_OUTPUT.md#sdl2-native-bridge). |
| [`fvm_swf_sound_mixer_create_configured`](../../../include/flashvm/swf_sound_mixer.h#L58) | Allocates fixed slots with explicit 48000 Hz conversion policy, here empty stereo silence; caller owns mixer. [Configured creation](../../../doc/SWF_SOUND_CONVERSION.md). |
| [`fvm_swf_sound_mixer_close`](../../../include/flashvm/swf_sound_mixer.h#L143) | Releases all software voices and slots after producer close; independent native ownership. [Mixer cleanup](../../../doc/SWF_SOUND_MIXER.md). |

[Catalog description](../../README.md#example-multimedia-swf_audio_queue).

## Optional release / Необязательное затухание v0.587

This explicit silence-only tutorial configures a 32-frame release before first
submission, calls release stop, then the original hard stop. It demonstrates
control/ownership and no nonzero seed; it is not audible fade acceptance.
[Detailed EN contract](../../../doc/PLAYER_AUDIO_RELEASE.md) /
[RU](../../../doc/PLAYER_AUDIO_RELEASE.ru.md).

| API | Individual behavior / Где читать |
| --- | --- |
| [`fvm_swf_sound_output_set_release`](../../../include/flashvm/swf_sound_output.h#L154) | Allocates optional bounded history transactionally before first submission; caller owns output, errors preserve its setting. [Contract](../../../doc/PLAYER_AUDIO_RELEASE.md#fvm_swf_sound_output_set_release). |
| [`fvm_swf_sound_output_stop_release`](../../../include/flashvm/swf_sound_output.h#L160) | Clears old voices/queue, optionally submits a tail without pausing; silence here needs no tail. [Status/retry/ownership](../../../doc/PLAYER_AUDIO_RELEASE.md#fvm_swf_sound_output_stop_release). |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_sdl_queue_close` | function | [include/flashvm/swf_sound_output.h:186](../../../include/flashvm/swf_sound_output.h#L186) |
| `fvm_audio_sdl_queue_open` | function | [include/flashvm/swf_sound_output.h:170](../../../include/flashvm/swf_sound_output.h#L170) |
| `fvm_swf_sound_mixer_close` | function | [include/flashvm/swf_sound_mixer.h:143](../../../include/flashvm/swf_sound_mixer.h#L143) |
| `fvm_swf_sound_mixer_create_configured` | function | [include/flashvm/swf_sound_mixer.h:58](../../../include/flashvm/swf_sound_mixer.h#L58) |
| `fvm_swf_sound_output_close` | function | [include/flashvm/swf_sound_output.h:163](../../../include/flashvm/swf_sound_output.h#L163) |
| `fvm_swf_sound_output_create` | function | [include/flashvm/swf_sound_output.h:50](../../../include/flashvm/swf_sound_output.h#L50) |
| `fvm_swf_sound_output_info` | function | [include/flashvm/swf_sound_output.h:148](../../../include/flashvm/swf_sound_output.h#L148) |
| `fvm_swf_sound_output_pause` | function | [include/flashvm/swf_sound_output.h:146](../../../include/flashvm/swf_sound_output.h#L146) |
| `fvm_swf_sound_output_pump` | function | [include/flashvm/swf_sound_output.h:124](../../../include/flashvm/swf_sound_output.h#L124) |
| `fvm_swf_sound_output_set_release` | function | [include/flashvm/swf_sound_output.h:154](../../../include/flashvm/swf_sound_output.h#L154) |
| `fvm_swf_sound_output_stop` | function | [include/flashvm/swf_sound_output.h:152](../../../include/flashvm/swf_sound_output.h#L152) |
| `fvm_swf_sound_output_stop_release` | function | [include/flashvm/swf_sound_output.h:160](../../../include/flashvm/swf_sound_output.h#L160) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../../../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
