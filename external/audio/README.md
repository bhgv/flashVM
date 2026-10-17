# Static Linux ALSA output SDK / Статический ALSA SDK

`FVM_ENABLE_LINUX_AUDIO` is OFF by default. Enabling it requires an explicit,
Linux-target-matching ALSA prefix with `include/alsa/asoundlib.h` and
`lib/libasound.a`. Configuration does not download anything and never substitutes
a system shared library. Vosk and speech recognition are not needed. The same
`FVM_ALSA_ROOT` can be shared with the optional Linux STT adapter.

Validated SDK: ALSA **1.2.12**, built from its
[official source archive](https://www.alsa-project.org/files/pub/lib/alsa-lib-1.2.12.tar.bz2).
Keep its upstream COPYING, LGPL notices and complete source when preparing a
distribution; the FlashVM MIT license does not replace the SDK's terms.
Typical SDK preparation from an independently acquired/extracted official source:

```sh
mkdir -p external/audio/work/alsa-build
cd external/audio/work/alsa-build
/absolute/alsa-lib-1.2.12/configure --prefix=/absolute/FlashVM/external/audio/install --disable-shared --enable-static
make -j4
make install
```

Use an absolute prefix and retain the configure summary. Depending on your target,
plugins/configuration files and transitive libraries may need explicit setup.
The SDK build must support S16 interleaved playback and the selected endpoint.
Sources/build/install belong under external; do not commit generated SDK files.
The example commands do not fetch or verify an archive automatically.

From the FlashVM root:

```sh
cmake -S . -B build-alsa -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DFVM_ENABLE_LINUX_AUDIO=ON -DFVM_ALSA_ROOT=/absolute/static-alsa-sdk -DCMAKE_BUILD_TYPE=Release
cmake --build build-alsa --target flashvm_tests_audio_alsa flashvm_tests_audio_alsa_faults flashvm_examples_multimedia_alsa_output_main --parallel
ctest --test-dir build-alsa -R audio_alsa --output-on-failure
./build-alsa/flashvm_examples_multimedia_alsa_output_main default
```

`FVM_ALSA_AUDIO_SYSTEM_LIBRARIES` accepts extra target static/system dependencies;
Threads, m and dl are already linked. There is no cross-platform ALSA fallback:
non-Linux/Android targets are rejected when the option is enabled. Disabled
builds keep the public factories and return UNSUPPORTED without opening a device.
No MEDIA/FFmpeg profile is required; decoded FFmpeg PCM can nevertheless feed the
same output if its geometry matches the configured provider.

The automated native tests use the real ALSA built-in null PCM with the explicit
`tests/alsa_null.conf` supplied by CTest. This validates real open/configure,
write, start, drop, prepare, close and context StopSounds without claiming audible
hardware output. An additional linker-wrapped test injects short write, EAGAIN,
zero progress and start failure while retaining the real null device. Physical
sound cards, desktop sound servers and other OS device playback remain open.

RU: реальный Linux-выход на статической ALSA, включается только явно. SDK задаётся
путём, автоматических скачиваний и подмены shared-библиотекой нет. Тесты используют
настоящий встроенный null PCM ALSA и отдельно подставленные I/O-ошибки; это не
проверка слышимого звука. Подготовьте устройства/права/конфигурацию ALSA сами и
выберите endpoint явно. Полные контракты и владение:
[ALSA_AUDIO_OUTPUT.md](../../doc/ALSA_AUDIO_OUTPUT.md).
