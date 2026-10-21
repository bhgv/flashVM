# Root PCM stream: color and tone

Bundled [pcm-stream.swf](pcm-stream.swf), standalone [Python3 generator](make_movie.py), [complete contract](../../../doc/PLAYER_PCM_STREAMS.md). This two-second SWF8 is 320×200 at 10 fps, one mono S16
LE/44100 stream with twenty blocks of 4410 samples. The first second has a blue background
and 441 Hz triangle tone, the second orange and 882 Hz. Integer-only generation is
deterministic and requires no compiler or third-party package. The movie is runnable;
physical output has not been verified in this environment.

The additional [pcm-segments.swf](pcm-segments.swf) changes its source at one second:
44100 Hz mono becomes 22050 Hz stereo, with an empty intermediate header in the
same frame. After the switch the left triangle is 882 Hz, the right 441 Hz.
The player retains one 48000 Hz stereo SDL queue and re-primes all headers on loop.
Generate it with `python3 flashVM/examples/player/pcm_stream/make_movie.py --segments`
and run the same player commands below with pcm-segments.swf instead of pcm-stream.swf.
Both generators are the same standalone script; default generation preserves the
original single-header bytes. Native queue tests compare every sample in both
movies against independent integer oracles. See the [header-sequence contract](../../../doc/PLAYER_PCM_STREAMS.md).

```sh
python3 flashVM/examples/player/pcm_stream/make_movie.py
cmake --build build/player --target flashvm_player
build/player/frontend/flashvm_player --interpreter --once flashVM/examples/player/pcm_stream/pcm-stream.swf
build/player/frontend/flashvm_player --jit --once flashVM/examples/player/pcm_stream/pcm-stream.swf
```

First configure the standalone player with a static SDL2 SDK, FVM_BUILD_DESKTOP_FRONTEND
and FVM_ENABLE_SDL_AUDIO using the [build guide](../../../doc/PLAYER_TEST_BUILDS.md); JIT needs an available backend. --mute displays
colors without an audio device. Omitting --once loops until close. --fps changes movie
authorization, not native sample rate; no time stretching is implemented. The host owns
the source snapshot until worker shutdown, pins the root across GC and keeps all VM
controls on the owner thread. Join precedes snapshot/device/context cleanup. The generator
has no callbacks or separately linked C wrappers. An IDE C89 host can pass the same bytes
through fvm_desktop_run_memory; see [embedded movie](../embedded_movie/README.md).

For reproducible queue checks use SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy with the
same arguments; this is not audible device evidence.
FVM_AUDIO_TRACE=/absolute/path/evidence explicitly enables WAV/text recording of accepted
PCM on close. This is not speaker recording. The worker has a frame gate, pause and
bounded work; exact A/V sync, stream+event mixing, partial ADPCM packets and other compressed
stream codecs remain open. Since v0.765 finite stream playback waits for authorized PCM and queue drain, with a 2000 ms extra deadline; unfinished work fails playback instead of reporting silent-success truncation.

## Whole-packet ADPCM variant (v0.764)

Bundled [adpcm-stream.swf](adpcm-stream.swf) is a standalone 320×200 SWF8, eight
frames at 5 fps. Each frame contains one four-bit mono ADPCM packet: 4096 source
samples at 22050 Hz, a modest approximately 441 Hz triangle, blue then orange.
The 1.6-second timeline and 32768-sample source duration intentionally differ;
no automatic drift correction or synthesized gap silence is claimed.

```sh
python3 flashVM/examples/player/pcm_stream/make_adpcm.py --ffmpeg ffmpeg
build/player/frontend/flashvm_player --interpreter --once flashVM/examples/player/pcm_stream/adpcm-stream.swf
build/player/frontend/flashvm_player --jit --once flashVM/examples/player/pcm_stream/adpcm-stream.swf
```

The separate [generator](make_adpcm.py) requires Python3 and an installed FFmpeg
with its `adpcm_swf` encoder and FLV muxer. Temporary raw/FLV files are cleaned on
success or failure; only the explicitly named bundled SWF is replaced, after
successful validation. Regeneration is deterministic for the selected encoder;
bytes need not be identical across encoder releases. Existing PCM generation is
unchanged and still needs no FFmpeg. **Playback needs no FFmpeg**: owner preparation
uses the C++98 decoder and the worker receives immutable native PCM. Snapshots
and SDL device remain owned until worker join. Only whole ADPCM packets are
supported, without root event-sound mixing; see [EN](../../../doc/STREAM_ADPCM.md) /
[RU](../../../doc/STREAM_ADPCM.ru.md). The independent optional player check is:

```sh
python3 flashVM/frontend/tests/stream_adpcm.py build/player/frontend/flashvm_player --ffmpeg
```

It decodes every bundled packet with FFmpeg, computes output conversion using
integer arithmetic and checks all 71332 accepted 48000 Hz stereo frames.
This is SDL dummy-queue evidence, not speaker recording or physical-device acceptance.
The normal C API source is `fvm_desktop_run_memory`; pass the same bundled bytes
through the [existing C89 embedding example](../embedded_movie/README.md).

Since v0.767 these commands also use the default [64-MiB aggregate preparation budget](../../../doc/PLAYER_STREAM_BUDGETS.md). Input files and expected samples are unchanged. The cap includes source expansion and retained predecessor ownership; it is not a process RSS limit.

## Stream recovery v0.769 / Восстановление очереди

Observed empty native stream queues with authorized work are paused, primed and
resumed by the worker. Initial/gated/finished streams are excluded. Existing
audio_underruns and optional bounded traces report observations, not DAC behavior.
Пустая очередь при разрешённых данных вызывает повторное наполнение; начало,
будущие кадры и EOF не считаются underrun. Ошибки backend передаются владельцу.
[EN](../../../doc/PLAYER_STREAM_RECOVERY.md) / [RU](../../../doc/PLAYER_STREAM_RECOVERY.ru.md).

From the component directory, request bounded evidence and run statistics explicitly:
Из каталога компонента можно явно запросить трассу и статистику:

```sh
FVM_AUDIO_TRACE=stream-recovery ./build-player/frontend/flashvm_player --once --stats examples/player/pcm_stream/adpcm-stream.swf
```

The run writes `stream-recovery.txt` and `stream-recovery.wav` on audio shutdown.
A well-scheduled run may have no stream_starved records; zero is a valid result.
The WAV contains accepted samples, including ones later discarded by navigation;
it is not a recording of speaker output. The deterministic regression forces
starvation without requiring CPU contention or a real audio device.
Трасса может не содержать stream_starved при штатной работе. WAV содержит принятые
отсчёты, а не запись динамиков; принятые до seek данные остаются в свидетельстве.
Для воспроизводимой проверки используется управляемое потребление тестового FIFO.

Windows x64 explicit Zig/static-SDL build instructions (v0.770): [EN](../../../doc/CROSS_WINDOWS_ZIG.md) / [RU](../../../doc/CROSS_WINDOWS_ZIG.ru.md). Build/link evidence remains separate from Windows execution and device acceptance. Кросс-сборка не заменяет запуск на Windows.
