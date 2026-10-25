# speech_macos — macOS: реальные speech и microphone permissions

Исходник prepare.mm для **настоящего macOS app host** (10.15+), а не переносимый
standalone executable. Он показывает то, что библиотека не должна делать скрыто:
реальные SFSpeechRecognizer/AVCaptureDevice permission prompts после явного
пользовательского действия, ожидание результата, затем open на main queue.

## Подготовка приложения / Host preparation

- Соберите CMake 3.16+ с настоящим Apple SDK, macOS deployment/runtime 10.15+, включённым
  FVM_ENABLE_MACOS_STT. Link frameworks: Speech, AVFoundation, Foundation;
  ApplicationServices используется TTS. Подключите static FlashVM к app target.
- Добавьте prepare.mm в Objective-C++ host target, сохраните C++98 для C++ части,
  `-fblocks -fno-objc-arc` для этого manual-reference-counted tutorial. Не
  компилируйте его Linux компилятором как обычный .cpp.
- В реальный Info.plist добавьте NSSpeechRecognitionUsageDescription и
  NSMicrophoneUsageDescription с понятным пользователю объяснением назначения.
  Подготовьте code signing и audio-input entitlement
  com.apple.security.device.audio-input в нужной sandbox/hardened-runtime политике.
- Подготовьте выбранный микрофон, system language/service и audible output voice.
  Main run loop должен работать всё время authorization/capture lifecycle.

## Реальное использование / Native use

Из main-thread пользовательского действия вызовите example_macos_prepare_stt с
callback и host userdata. Функция возвращает только scheduling result. Реальный
speech grant сначала проверяется системой; при отказе callback получает
UNSUPPORTED и null session. Затем запрашивается microphone grant. Только после
обоих grants на main queue создаётся native backend/session. Callback получает
actual open status и **владеет** ненулевым успешным session; poll query/copy и
закрытие выполняются по [STT контракту](../../../doc/SPEECH_BRIDGES.md#fvm_stt_poll).
Host userdata должен оставаться живым до завершения обоих async запросов.
Callback не должен немедленно закрывать session, если хочет продолжать listening.

example_macos_open_tts показывает отдельно настоящий голос: main-thread factory
и open без speech-recognition/microphone prompts. Затем speak, ожидание выбранного
host completion/stop момента, stop/close. Немедленный close отменяет queued text.

[GC:] Нативные sessions и Objective-C callback captures не являются корнями VM.
Не храните VM values в userdata без явных roots; VM вызовы остаются на её потоке.
Сами реальные prompts принадлежат host example, native speech API их не вызывает.

## Проверка и ограничения / Validation and limits

Apple compile, app signing, grant/deny flows и native microphone/voice runs ещё
не выполнены этим Linux runner. Исходник использует реальные SDK функции, но
проверка Linux guarded sources не означает готовность macOS release. Для ручной
приёмки проверьте denial/restricted, отсутствие input, service unavailable,
partial/final Unicode, close while recognizing и отсутствие утечки microphone
после закрытия. Provider может требовать сеть; offline режим не обещан.


Исходники: [prepare.mm](prepare.mm). Каждая непустая строка учебного C/C++/Objective-C++/Java кода
пояснена; ошибок через assertions здесь нет.

## Сборка и запуск / Build and run

Собирается внутри подготовленного native приложения соответствующей ОС согласно
описанным выше требованиям. Корневая CMake сборка не выдаёт этот файл за desktop
main или автоматически протестированный app. Нет отдельного root example target.


## Демонстрируемые API / Demonstrated API

| Function (source declarations linked below) | Detailed guide file and section |
| --- | --- |
| [`fvm_stt_open`](../../../include/flashvm/speech.h#L119) | [SPEECH_BRIDGES.md — fvm_stt_open](../../../doc/SPEECH_BRIDGES.md#fvm_stt_open) |
| [`fvm_stt_system_backend`](../../../include/flashvm/speech.h#L92) | [SPEECH_BRIDGES.md — fvm_stt_system_backend](../../../doc/SPEECH_BRIDGES.md#fvm_stt_system_backend) |
| [`fvm_tts_open`](../../../include/flashvm/speech.h#L103) | [SPEECH_BRIDGES.md — fvm_tts_open](../../../doc/SPEECH_BRIDGES.md#fvm_tts_open) |
| [`fvm_tts_system_backend`](../../../include/flashvm/speech.h#L87) | [SPEECH_BRIDGES.md — fvm_tts_system_backend](../../../doc/SPEECH_BRIDGES.md#fvm_tts_system_backend) |

Структуры [FVM_TtsBackend](../../../include/flashvm/speech.h#L14), [FVM_SttBackend](../../../include/flashvm/speech.h#L26), [FVM_AndroidSpeechConfig](../../../include/flashvm/speech.h#L68),
[FVM_LinuxSttConfig](../../../include/flashvm/speech.h#L55) и ownership/status contracts:
[SPEECH_BRIDGES.md](../../../doc/SPEECH_BRIDGES.md),
[public speech.h](../../../include/flashvm/speech.h).
Все примеры различают real native operations, deterministic fixtures и пока
не выполненные platform/device acceptance runs.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AndroidSpeechConfig` | struct | [include/flashvm/speech.h:68](../../../include/flashvm/speech.h#L68) |
| `FVM_LinuxSttConfig` | struct | [include/flashvm/speech.h:55](../../../include/flashvm/speech.h#L55) |
| `fvm_stt_open` | function | [include/flashvm/speech.h:119](../../../include/flashvm/speech.h#L119) |
| `fvm_stt_poll` | function | [include/flashvm/speech.h:123](../../../include/flashvm/speech.h#L123) |
| `fvm_stt_system_backend` | function | [include/flashvm/speech.h:92](../../../include/flashvm/speech.h#L92) |
| `FVM_SttBackend` | struct | [include/flashvm/speech.h:26](../../../include/flashvm/speech.h#L26) |
| `fvm_tts_open` | function | [include/flashvm/speech.h:103](../../../include/flashvm/speech.h#L103) |
| `fvm_tts_system_backend` | function | [include/flashvm/speech.h:87](../../../include/flashvm/speech.h#L87) |
| `FVM_TtsBackend` | struct | [include/flashvm/speech.h:14](../../../include/flashvm/speech.h#L14) |
<!-- source-links:end -->
