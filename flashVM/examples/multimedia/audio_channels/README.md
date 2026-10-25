# Аудиоканалы и StopSounds / Audio channels

Исполняемый C89-пример связывает контекст VM с настоящим файловым PCM-выходом,
записывает два нулевых mono S16 sample frames, затем исполняет AVM1 StopSounds.
Callback остановки действительно закрывает FILE через fclose. Это полезный
маленький шаблон регистрации своего системного аудиовыхода: замените fwrite на
синхронную передачу PCM своей библиотеке, а fclose — на stop/discard её очереди.
Файловый sink не воспроизводит звук и не подтверждает работу звуковой карты.

## Сборка и запуск

```sh
cmake -S . -B build-audio -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-audio --target flashvm_examples_multimedia_audio_channels_main --parallel
./build-audio/flashvm_examples_multimedia_audio_channels_main /tmp/channel-output.pcm
```

Работает и с NO_MEDIA: FFmpeg и устройства не нужны. Без аргумента выводится
usage, файл не открывается. С аргументом добавляет 4 байта в названный файл;
существующий файл дополняется. Ожидается `Active: 0; stop callbacks: 1; appended
bytes: 4` на обычной платформе с 16-битным short. Формат raw, без WAV-заголовка:
mono, 8000 Hz, native-endian signed-16. На Windows передайте подходящий локальный
путь вместо `/tmp/...`; остальные native ОС этим запуском не сертифицированы.

## Устройство и владение

[main.c](main.c) подробно комментирует каждую непустую строку. Таблица output
копируется в контекст; userdata остаётся адресом host-структуры, которая живёт
до конца context_destroy. Канал сначала inactive, становится active только
после успешного write. Action 0x09 останавливает все активные регистрации
именно этого контекста. Успешный close не вызывает повторный stop и инвалидирует
handle. Если write частично открыл файл и завершился ошибкой, host cleanup
отдельно закрывает такой ресурс после уничтожения контекста. Stop error не
маскируется; в реальном host-приложении повторите stop/close либо выполните
собственный гарантированный device cleanup.

Никаких assert/abort, неявных разрешений, устройств, потоков и сети. При замене
sink на системную библиотеку host заранее открывает/авторизует устройство,
согласует S16/native-endian/sample rate/channels, копирует borrowed данные до
возврата callback и синхронно отбрасывает очередь при stop. Callback не должен
менять реестр каналов, запускать GC или бросать C++ исключения.

## Демонстрируемые API

- [`fvm_audio_channel_create`](../../../include/flashvm/multimedia.h#L100) — копирование output и публикация inactive: [контракт](../../../doc/AUDIO_CHANNELS.md#fvm_audio_channel_create).
- [`fvm_audio_channel_write`](../../../include/flashvm/multimedia.h#L104) — проверенный borrowed PCM: [контракт](../../../doc/AUDIO_CHANNELS.md#fvm_audio_channel_write).
- [`fvm_audio_channel_active`](../../../include/flashvm/multimedia.h#L116) — диагностический флаг: [контракт](../../../doc/AUDIO_CHANNELS.md#fvm_audio_channel_active).
- [`fvm_audio_channel_close`](../../../include/flashvm/multimedia.h#L112) — остановка и снятие регистрации: [контракт](../../../doc/AUDIO_CHANNELS.md#fvm_audio_channel_close).
- [`fvm_avm1_execute`](../../../include/flashvm/avm1.h#L46) — действительное исполнение StopSounds: [общая семантика](../../../doc/AUDIO_CHANNELS.md#scope).
- [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70), [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74), [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82), [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86), [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) — runtime/context и порядок cleanup: [API index](../../../doc/API_INDEX.md#runtime-and-context), [terminal audio cleanup](../../../doc/AUDIO_CHANNELS.md#context-teardown).

Связанные, но не вызываемые в этом примере: [`fvm_audio_stop_all`](../../../include/flashvm/multimedia.h#L120),
[`fvm_audio_channel_stop`](../../../include/flashvm/multimedia.h#L108) и script `stopAllSounds()` — [полное руководство](../../../doc/AUDIO_CHANNELS.md).

## English

A line-commented C89 executable submits two silent mono S16 samples to an
explicit raw PCM file and closes the actual host FILE through AVM1 StopSounds.
It demonstrates copied callbacks, borrowed buffers, context isolation and
cleanup ownership, with normal error handling. No-argument invocation has no
file/device side effects. The example appends four bytes; it is a file sink,
not audible playback or a native sound-device acceptance test. Replace write
and stop with your real library operations while preserving synchronous borrowed
buffer consumption and stop/discard semantics. Host userdata must outlive context
teardown; failed-write resources remain explicitly host-owned.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_active` | function | [include/flashvm/multimedia.h:116](../../../include/flashvm/multimedia.h#L116) |
| `fvm_audio_channel_close` | function | [include/flashvm/multimedia.h:112](../../../include/flashvm/multimedia.h#L112) |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:100](../../../include/flashvm/multimedia.h#L100) |
| `fvm_audio_channel_stop` | function | [include/flashvm/multimedia.h:108](../../../include/flashvm/multimedia.h#L108) |
| `fvm_audio_channel_write` | function | [include/flashvm/multimedia.h:104](../../../include/flashvm/multimedia.h#L104) |
| `fvm_audio_stop_all` | function | [include/flashvm/multimedia.h:120](../../../include/flashvm/multimedia.h#L120) |
| `fvm_avm1_execute` | function | [include/flashvm/avm1.h:46](../../../include/flashvm/avm1.h#L46) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
<!-- source-links:end -->
