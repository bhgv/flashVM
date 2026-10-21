# Root PCM stream: color and tone

Bundled [pcm-stream.swf](pcm-stream.swf), standalone [Python3 generator](make_movie.py), [complete contract](../../../doc/PLAYER_PCM_STREAMS.md). This two-second SWF8 is 320×200 at 10 fps, one mono S16 LE/44100 stream with twenty blocks of 4410 samples. The first second has a blue background and 441 Hz triangle tone, the second orange and 882 Hz. Integer-only generation is deterministic and requires no compiler or third-party package. The movie is runnable; physical output has not been verified in this environment.

```sh
python3 flashVM/examples/player/pcm_stream/make_movie.py
cmake --build build/player --target flashvm_player
build/player/frontend/flashvm_player --interpreter --once flashVM/examples/player/pcm_stream/pcm-stream.swf
build/player/frontend/flashvm_player --jit --once flashVM/examples/player/pcm_stream/pcm-stream.swf
```

First configure the standalone player with a static SDL2 SDK, FVM_BUILD_DESKTOP_FRONTEND and FVM_ENABLE_SDL_AUDIO using the [build guide](../../../doc/PLAYER_TEST_BUILDS.md); JIT needs an available backend. --mute displays colors without an audio device. Omitting --once loops until close. --fps changes movie authorization, not native sample rate; no time stretching is implemented. The host owns the source snapshot until worker shutdown, pins the root across GC and keeps all VM controls on the owner thread. Join precedes snapshot/device/context cleanup. The generator has no callbacks or separately linked C wrappers. An IDE C89 host can pass the same bytes through fvm_desktop_run_memory; see [embedded movie](../embedded_movie/README.md).

For reproducible queue checks use SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy with the same arguments; this is not audible device evidence. FVM_AUDIO_TRACE=/absolute/path/evidence explicitly enables WAV/text recording of accepted PCM on close. This is not speaker recording. The worker has a frame gate, pause and bounded work; exact A/V sync, stream+event mixing, multiple headers and compressed stream codecs remain open.
