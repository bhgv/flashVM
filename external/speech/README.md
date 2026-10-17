# Native speech dependencies / Зависимости системной речи

The ordinary TTS/STT callback ABI requires no FFmpeg or third-party speech SDK.
Windows uses installed SAPI/COM (`sapi`, `ole32`, `uuid`, `user32`); macOS uses
ApplicationServices for TTS and optional Speech/AVFoundation/Foundation for STT.
Android uses its NDK JNI interface plus the supplied real Java service helpers
compiled into the host app. Linux TTS talks SSIP directly to an explicitly chosen
Speech Dispatcher Unix socket, so no client library/shared-library loader or
command subprocess is required. The binary-safe fvm_tts_unix_backend also
supports explicit abstract endpoints on Linux/Android; this requires a real
SSIP service there, not merely an available Unix socket namespace.

## Optional static Linux recognition

Linux has no universal OS dictation service. The provided real capture/inference
adapter uses **ALSA + Vosk**, an explicitly installed offline model and an explicit
capture device. Default builds leave this optional SDK off and return
FVM_UNSUPPORTED from the Linux STT factory. They do not fabricate transcripts.

```sh
cmake -S . -B build-speech -DFVM_DEFS_FILE=cmake/profiles/minimal.defs \
  -DFVM_ENABLE_LINUX_STT=ON \
  -DFVM_VOSK_ROOT=/absolute/static-vosk-sdk \
  -DFVM_ALSA_ROOT=/absolute/static-alsa-sdk \
  -DFVM_VOSK_SYSTEM_LIBRARIES="/absolute/transitive-dependency-a.a;/absolute/transitive-dependency-b.a"
cmake --build build-speech --target flashvm_examples_multimedia_speech_stt_main
./build-speech/flashvm_examples_multimedia_speech_stt_main \
  --linux /absolute/installed-vosk-model explicit-alsa-device
```

The dependency-list values above are **placeholders**, not a tested universal
Vosk link line. Supply the complete static Kaldi/OpenFST/BLAS/etc. dependency set
required by your own SDK. CMake checks `include/vosk_api.h`, `lib/libvosk.a`,
`include/alsa/asoundlib.h`, `lib/libasound.a`; it never substitutes `.so` files.
Vendor sources/builds/models belong under `external/` or an explicit SDK prefix,
not `src/multimedia`. This folder's ignored `work/` and `install/` are convenient
local dependency locations. No source, model or shared binary is downloaded by
FlashVM configuration.

Pinned API compatibility used for source validation: Vosk **v0.3.45**
[official header](https://github.com/alphacep/vosk-api/blob/v0.3.45/src/vosk_api.h),
ALSA **1.2.12** [official source](https://www.alsa-project.org/files/pub/lib/alsa-lib-1.2.12.tar.bz2).
This checkpoint compiled the four Linux SDK adapter translation units against
these real headers and built ALSA statically. A complete static Vosk SDK/model and
physical microphone were not available for an end-to-end run. Do not report that
compile check as tested speech recognition. Upstream Vosk packages often provide
shared libraries; they are **not** silently accepted as the required static SDK.
Use upstream's build instructions to produce an appropriate target SDK and
preserve all model/dependency licenses. No universal static Vosk build recipe is
claimed here.

Native C++98 adapters use Vosk's C ABI, not its internal C++ types. ALSA captures
native signed-16 mono at 16000 Hz; Vosk receives the short-sample entrypoint so
byte order is not guessed. Nonblocking capture processes at most 2048 sample
frames per poll. Real model loading/inference may take significant time on the
calling host thread; a host worker/owned-copy queue is an application policy.

RU: это реальный опциональный ALSA/Vosk переходник, а не системная служба,
присутствующая на любом Linux. Модель и устройство задаются явно. Источники,
статические SDK и модели нужно подготовить самостоятельно; отсутствие SDK даёт
UNSUPPORTED. Проверка компиляции не заменяет запуск с настоящим микрофоном.

Full ABI, platform setup and examples: [SPEECH_BRIDGES.md](../../doc/SPEECH_BRIDGES.md).
