# speech_stt — Системный STT: разрешения и захват настоящей речи

Исполняемый C89 host: явно открывает настоящий микрофон/recognizer, до примерно
30 секунд опрашивает actual partial/final текст, закрывается после final либо
ошибки/timeout. Ни одна заранее заданная строка не выдаётся за распознавание.
Без аргументов только usage, без capture/permission dialogs. [GC:] Микрофон,
модель и host transcript allocations не принадлежат VM collector.

## Подготовка под ОС / System preparation

**Windows:** подготовьте SAPI recognizer language, выбранный системный микрофон
и реальные разрешения приложения. Нужен SDK с sapi.h и COM/SAPI libs. Запуск
`--system` создаёт STA, real shared recognizer и активную dictation grammar.
Host tick обрабатывает Windows messages и ждёт 20 ms, чтобы не блокировать native
уведомления. Скажите фразу; ожидайте actual final transcript. Close деактивирует
только grammar этого session. Нет engine/permission/device — честная ошибка.
Native Windows execution ещё не проверен этим runner.

**Linux:** подготовьте **статические** ALSA и Vosk SDK с transitive libraries,
включите FVM_ENABLE_LINUX_STT и установите совместимую offline модель. Модель не
скачивается и не выбирается автоматически. Подготовьте явно выбранный и
разрешённый ALSA input. Запуск: `--linux /actual/vosk-model explicit-alsa-device`.
Настройка SDK описана в [external/speech/README.md](../../../external/speech/README.md).
Реальный capture — native S16 mono 16000 Hz; inference обрабатывает до 2048 frames
за poll. Вызывающий поток должен опрашивать достаточно часто; долгие паузы могут
дать capture overrun. Тогда пример сообщает ошибку и закрывает input, не
подменяет устройство. Без optional SDK factory возвращает UNSUPPORTED.
Компиляция переходников с реальными headers проверена, запуск с моделью и
микрофоном пока нет.

**macOS 10.15+:** включите FVM_ENABLE_MACOS_STT, подготовьте app bundle, usage
strings, подпись/entitlements, получите speech+microphone grants на main thread.
Точный порядок и реальный код permission prompts:
[speech_macos](../speech_macos/README.md). Сам native open не запрашивает grants.
Console `--system` пригоден только для уже правильно подготовленного host;
обычный неподготовленный Terminal binary не считается готовым bundle. Tick
крутит CFRunLoop; native task/tap очищаются при close. Apple run не проверен.

**Android:** используйте [speech_android](../speech_android/README.md), настоящий
application Context, global class references, RECORD_AUDIO declaration и runtime
grant, installed recognition provider и visibility query. Helper активирует
реальный SpeechRecognizer только после явного open. Offline preference advisory;
обработка определяется provider. Не пытайтесь получить Context desktop main.

## Реализация и ожидаемый результат / Implementation and expected result

Сначала query poll с capacity=0 получает required including NUL и сохраняет
**тот же** transcript; malloc создаёт host copy, второй poll копирует его.
Final/partial выводятся явно. Текст освобождается сразу, session — на общем пути
cleanup. Последующие callbacks не заменяют уже queried snapshot. No-match,
ошибка или истечение окна без final дают ненулевой exit и диагноз, не фиктивный
успех. POSIX timer/Win32 pump/CFRunLoop — политика standalone host, не скрытый
поток speech библиотеки.

English: real explicit capture, per-OS permission/SDK/model preparation, bounded
host polling, retained query/copy transcript, ordinary errors and deterministic
cleanup. A smoke run never opens a microphone and is not hardware evidence.


Исходники: [main.c](main.c). Каждая непустая строка учебного C/C++/Objective-C++/Java кода
пояснена; ошибок через assertions здесь нет.

## Сборка и запуск / Build and run

```sh
cmake -S . -B build-speech -DFVM_DEFS_FILE=cmake/profiles/minimal.defs
cmake --build build-speech --target flashvm_examples_multimedia_speech_stt_main
./build-speech/flashvm_examples_multimedia_speech_stt_main
```

No-argument smoke output is usage only. Manual platform arguments and SDK flags
are described above; Linux STT additionally needs the explicit optional static SDK.


## Демонстрируемые API / Demonstrated API

| Function (source declarations linked below) | Detailed guide file and section |
| --- | --- |
| [`fvm_stt_close`](../../../include/flashvm/speech.h#L106) | [SPEECH_BRIDGES.md — fvm_stt_close](../../../doc/SPEECH_BRIDGES.md#fvm_stt_close) |
| [`fvm_stt_linux_backend`](../../../include/flashvm/speech.h#L57) | [SPEECH_BRIDGES.md — fvm_stt_linux_backend](../../../doc/SPEECH_BRIDGES.md#fvm_stt_linux_backend) |
| [`fvm_stt_open`](../../../include/flashvm/speech.h#L100) | [SPEECH_BRIDGES.md — fvm_stt_open](../../../doc/SPEECH_BRIDGES.md#fvm_stt_open) |
| [`fvm_stt_poll`](../../../include/flashvm/speech.h#L102) | [SPEECH_BRIDGES.md — fvm_stt_poll](../../../doc/SPEECH_BRIDGES.md#fvm_stt_poll) |
| [`fvm_stt_system_backend`](../../../include/flashvm/speech.h#L83) | [SPEECH_BRIDGES.md — fvm_stt_system_backend](../../../doc/SPEECH_BRIDGES.md#fvm_stt_system_backend) |

Структуры [FVM_TtsBackend](../../../include/flashvm/speech.h#L13), [FVM_SttBackend](../../../include/flashvm/speech.h#L24), [FVM_AndroidSpeechConfig](../../../include/flashvm/speech.h#L60),
[FVM_LinuxSttConfig](../../../include/flashvm/speech.h#L49) и ownership/status contracts:
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
| `FVM_AndroidSpeechConfig` | struct | [include/flashvm/speech.h:60](../../../include/flashvm/speech.h#L60) |
| `FVM_LinuxSttConfig` | struct | [include/flashvm/speech.h:49](../../../include/flashvm/speech.h#L49) |
| `fvm_stt_close` | function | [include/flashvm/speech.h:106](../../../include/flashvm/speech.h#L106) |
| `fvm_stt_linux_backend` | function | [include/flashvm/speech.h:57](../../../include/flashvm/speech.h#L57) |
| `fvm_stt_open` | function | [include/flashvm/speech.h:100](../../../include/flashvm/speech.h#L100) |
| `fvm_stt_poll` | function | [include/flashvm/speech.h:102](../../../include/flashvm/speech.h#L102) |
| `fvm_stt_system_backend` | function | [include/flashvm/speech.h:83](../../../include/flashvm/speech.h#L83) |
| `FVM_SttBackend` | struct | [include/flashvm/speech.h:24](../../../include/flashvm/speech.h#L24) |
| `FVM_TtsBackend` | struct | [include/flashvm/speech.h:13](../../../include/flashvm/speech.h#L13) |
<!-- source-links:end -->
