# Linux ALSA output / Нативный аудиовыход

Исполняемый C89-пример явно открывает заданный ALSA PCM endpoint, передаёт короткий
тон через зарегистрированный канал VM, затем по Enter исполняет StopSounds.
Назначение: показать связку настоящей системной аудиобиблиотеки с C89 callback ABI
и правильное разделение lifetime provider/channel/context. Это не SWF Sound и не
полный аудиоплеер; настройка endpoint остаётся у хоста.

## Подготовка системы

Нужен Linux, подходящий **статический ALSA SDK**, конфигурация ALSA и доступ к
выбранному PCM. Инструкции SDK: [external/audio](../../../external/audio/README.md).
Обычная NO_MEDIA-сборка достаточна; Vosk/FFmpeg не нужны. Для настоящего устройства
проверьте ACL `/dev/snd`, desktop session и установленные плагины звукового сервера.
`default` зависит от host-конфигурации, `plughw:0,0` — пример, а не гарантированное
имя вашей карты. Не запрашиваются скрытые разрешения и не запускаются службы.
На Windows/macOS/Android этот ALSA-переходник выключен и возвращает UNSUPPORTED.

```sh
cmake -S . -B build-alsa -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DFVM_ENABLE_LINUX_AUDIO=ON -DFVM_ALSA_ROOT=/absolute/static-alsa-sdk -DCMAKE_BUILD_TYPE=Release
cmake --build build-alsa --target flashvm_examples_multimedia_alsa_output_main --parallel
./build-alsa/flashvm_examples_multimedia_alsa_output_main default
```

Переданный endpoint используется явно; на реальном устройстве будет короткий
звук. Без аргументов выводится только usage, устройства не открываются.
Тон: 250 Hz square wave, 800 mono sample frames при 8000 Hz (0.1 секунды),
умеренная S16 амплитуда; buffer latency запрашивается 200000 микросекунд.
После отправки Enter или EOF вызывает остановку. На built-in null PCM ALSA
звук не слышен по определению; для такого запуска нужна явная конфигурация с
этим endpoint, например `ALSA_CONFIG_PATH=.../tests/alsa_null.conf` и имя
`flashvm_null`. Это библиотечная проверка, не приёмка звуковой карты.

## Реализация и cleanup

[main.c](main.c) комментирует каждую непустую строку и использует обычные ошибки,
без assert/abort. Сначала открывается provider и создаётся VM, затем output-table
копируется в channel. Локальный PCM-буфер заимствуется синхронно. Сам provider
остаётся host-owned: нельзя закрывать его, пока channel хранит его userdata.
После успешной остановки close снимает регистрацию. Cleanup всегда уничтожает
context перед native provider, чтобы даже ошибочный stop имел живой userdata.
Native close инвалидирует provider даже при ошибке; повторять старый handle нельзя.

Write использует nonblocking ALSA: short writes продвигаются, EAGAIN/нулевой
прогресс и ошибки возвращаются после best-effort drop, без ожидания и таймера.
После stop следующий write делает prepare. Уже прозвучавший фрагмент при частичной
ошибке отменить нельзя. Пример не скрывает эти ошибки и не выдаёт null PCM за звук.
Сама библиотека не читает stdin: ожидание Enter выполняет только учебный host.

## Демонстрируемые функции API

- [`fvm_audio_linux_alsa_open`](../../../include/flashvm/multimedia.h#L91) — явное устройство/формат/латентность и callbacks: [контракт open](../../../doc/ALSA_AUDIO_OUTPUT.md#fvm_audio_linux_alsa_open).
- [`fvm_audio_linux_alsa_close`](../../../include/flashvm/multimedia.h#L95) — terminal close после снятия каналов: [контракт close](../../../doc/ALSA_AUDIO_OUTPUT.md#fvm_audio_linux_alsa_close).
- [`fvm_audio_channel_create`](../../../include/flashvm/multimedia.h#L100), [`fvm_audio_channel_write`](../../../include/flashvm/multimedia.h#L104), [`fvm_audio_channel_close`](../../../include/flashvm/multimedia.h#L112) — регистрация, borrowed PCM, снятие: [AUDIO_CHANNELS.md](../../../doc/AUDIO_CHANNELS.md#fvm_audio_channel_create).
- [`fvm_avm1_execute`](../../../include/flashvm/avm1.h#L46) — настоящий StopSounds: [семантика](../../../doc/AUDIO_CHANNELS.md#scope).
- [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70), [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74), [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82), [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86), [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) — VM и порядок владения: [API index](../../../doc/API_INDEX.md#runtime-and-context), [terminal channel cleanup](../../../doc/AUDIO_CHANNELS.md#context-teardown).

## English

An explicit Linux PCM endpoint receives a short generated 250 Hz mono S16 tone;
the host waits for Enter and executes AVM1 StopSounds through a context channel.
Build with the supplied static ALSA SDK; no FFmpeg/Vosk or VM movie is required.
Prepare device ACLs, ALSA configuration and any host-selected sound-server plugin
before running. No-argument usage performs no device work. The native provider
outlives channel/context cleanup; callbacks consume borrowed PCM synchronously.
Nonblocking output errors propagate after best-effort drop without hidden retries.
The real ALSA null PCM is useful for library validation and produces no audible
sound; it does not certify physical playback or another operating system.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_close` | function | [include/flashvm/multimedia.h:112](../../../include/flashvm/multimedia.h#L112) |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:100](../../../include/flashvm/multimedia.h#L100) |
| `fvm_audio_channel_write` | function | [include/flashvm/multimedia.h:104](../../../include/flashvm/multimedia.h#L104) |
| `fvm_audio_linux_alsa_close` | function | [include/flashvm/multimedia.h:95](../../../include/flashvm/multimedia.h#L95) |
| `fvm_audio_linux_alsa_open` | function | [include/flashvm/multimedia.h:91](../../../include/flashvm/multimedia.h#L91) |
| `fvm_avm1_execute` | function | [include/flashvm/avm1.h:46](../../../include/flashvm/avm1.h#L46) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
<!-- source-links:end -->
