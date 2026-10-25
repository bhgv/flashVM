# speech_tts — Системный TTS: подготовка голоса и реальная речь

Исполняемый C89 пример: заводит настоящий голос/SSIP session, отправляет явный
UTF-8 текст, ждёт Enter, затем останавливает и закрывает session. Это ожидание
нужно: немедленный close отменил бы только что поставленную речь. Без аргументов
пример выводит usage и ничего не произносит/не подключает. Обычные ошибки вместо
assertions; [GC:] native sessions не входят в VM heap и освобождаются явно.

## Подготовка под ОС / System preparation

**Windows:** подготовьте установленный SAPI голос и работающий выход звука;
соберите Windows SDK с sapi.h. CMake подключает sapi/ole32/uuid/user32. Вызовы идут
на создающем STA потоке; конфликт с MTA даёт UNSUPPORTED. Запустите бинарник с
`--system "Hello from the system voice"`; после прослушивания нажмите Enter.
Нативные COM ссылки и успешный CoInitializeEx балансируются на close.
Windows non-ASCII argv должен действительно содержать UTF-8 (host manifest/codepage
или явное преобразование wide command line); один main(char**) не превращает
ANSI аргументы в UTF-8. Приведённый Windows Hello пример использует ASCII. Микрофон
TTS не требуется. Native Windows run ещё не проверен этим Linux runner.

**Linux:** установите и настройте настоящий Speech Dispatcher с рабочим output
module средствами своей ОС; запустите службу штатным способом. Определите её
реальный Unix socket и права текущего пользователя. Этот пример не угадывает
путь и не запускает daemon. `test -S /actual/speechd/socket` проверяет тип пути;
затем передайте `--linux /actual/speechd/socket "Привет"`. Ожидается настоящая
системная речь, но это ручная проверка с реальной службой. Пять секунд — timeout
отдельного socket I/O, не время завершения воспроизведения. В тестах socketpair
проверяется протокол, а не звуковая карта.

**macOS:** подготовьте установленный системный голос и audible output; нужен
ApplicationServices. Запустите `--system "Hello"` на основном host потоке и
держите session до Enter. Для app-интеграции см.
[подготовку macOS](../speech_macos/README.md). Здесь используется настоящий,
хотя deprecated, Speech Synthesis C API; полная AVSpeech API совместимость не
заявлена. Native Apple build/run остаётся отдельной проверкой.

**Android:** desktop main не заменяет приложение с Context/Looper. Используйте
[Android host/JNI пример](../speech_android/README.md), добавьте helpers в app,
подготовьте voice service и visibility queries, дождитесь TTS OnInit. Пока голос
не готов, speak возвращает NOT_FOUND: host сохраняет текст и явно повторяет
запрос. TTS не запрашивает RECORD_AUDIO.

## Abstract Unix names / Имена без файлового пути

Android/Linux допускают именованные abstract sockets. Для такого **реального
SSIP endpoint** используйте `--abstract service-name "Привет"`. В argv передаётся
имя без ведущего NUL и без условного `@`; [fvm_tts_unix_backend](../../../include/flashvm/speech.h#L52) получает
[FVM_SpeechUnixEndpoint](../../../include/flashvm/speech.h#L42) с namespace=[FVM_SPEECH_UNIX_ABSTRACT](../../../include/flashvm/speech.h#L40). Native переходник сам
вставляет sun_path[0]=0 и передаёт точную длину. CLI не может передать embedded NUL;
C host задаёт byte array и length, где внутренние/последние NUL значимы. Для пути
по-прежнему есть --linux path text. Не считайте любой abstract service сервером
SSIP: он обязан реально понимать протокол. Android стандартный TextToSpeech
использует свой JNI backend, не произвольный socket.

API: [fvm_tts_unix_backend](../../../include/flashvm/speech.h#L52) — [SPEECH_BRIDGES.md](../../../doc/SPEECH_BRIDGES.md#fvm_tts_unix_backend),
структура [FVM_SpeechUnixEndpoint](../../../include/flashvm/speech.h#L42) и [правила namespace](../../../doc/SPEECH_BRIDGES.md#именованные-unix-сокеты-androidlinux--named-unix-socket-namespaces).

## Реализация и результат / Implementation and result

Factory не открывает ресурс; [fvm_tts_open](../../../include/flashvm/speech.h#L103) делает это явно. Backend таблица
копируется, путь Linux borrowed до open и хранится argv весь запуск. Speak
проверяет Unicode и копирует/передаёт текст native provider. OK означает enqueue,
не audible completion. Ошибки open/speak/stop печатаются; cleanup выполняется
после успеха и ошибок. Close не возвращает ошибку, поэтому пример отдельно
вызывает stop. Linux CANCEL SELF не останавливает речь других приложений.

English: executable real native voice/SSIP tutorial, explicit per-OS preparation,
manual lifetime through playback, normal errors, no assertions and no hidden
microphone. No-argument smoke test is silent, not audible acceptance evidence.


Исходники: [main.c](main.c). Каждая непустая строка учебного C/C++/Objective-C++/Java кода
пояснена; ошибок через assertions здесь нет.

## Сборка и запуск / Build and run

```sh
cmake -S . -B build-speech -DFVM_DEFS_FILE=cmake/profiles/minimal.defs
cmake --build build-speech --target flashvm_examples_multimedia_speech_tts_main
./build-speech/flashvm_examples_multimedia_speech_tts_main
```

No-argument smoke output is usage only. Manual platform arguments and SDK flags
are described above; Linux STT additionally needs the explicit optional static SDK.


## Демонстрируемые API / Demonstrated API

| Function (source declarations linked below) | Detailed guide file and section |
| --- | --- |
| [`fvm_tts_close`](../../../include/flashvm/speech.h#L115) | [SPEECH_BRIDGES.md — fvm_tts_close](../../../doc/SPEECH_BRIDGES.md#fvm_tts_close) |
| [`fvm_tts_linux_backend`](../../../include/flashvm/speech.h#L97) | [SPEECH_BRIDGES.md — fvm_tts_linux_backend](../../../doc/SPEECH_BRIDGES.md#fvm_tts_linux_backend) |
| [`fvm_tts_open`](../../../include/flashvm/speech.h#L103) | [SPEECH_BRIDGES.md — fvm_tts_open](../../../doc/SPEECH_BRIDGES.md#fvm_tts_open) |
| [`fvm_tts_speak`](../../../include/flashvm/speech.h#L107) | [SPEECH_BRIDGES.md — fvm_tts_speak](../../../doc/SPEECH_BRIDGES.md#fvm_tts_speak) |
| [`fvm_tts_stop`](../../../include/flashvm/speech.h#L111) | [SPEECH_BRIDGES.md — fvm_tts_stop](../../../doc/SPEECH_BRIDGES.md#fvm_tts_stop) |
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
| `FVM_SPEECH_UNIX_ABSTRACT` | macro / define | [include/flashvm/speech.h:40](../../../include/flashvm/speech.h#L40) |
| `FVM_SpeechUnixEndpoint` | struct | [include/flashvm/speech.h:42](../../../include/flashvm/speech.h#L42) |
| `FVM_SttBackend` | struct | [include/flashvm/speech.h:26](../../../include/flashvm/speech.h#L26) |
| `fvm_tts_close` | function | [include/flashvm/speech.h:115](../../../include/flashvm/speech.h#L115) |
| `fvm_tts_linux_backend` | function | [include/flashvm/speech.h:97](../../../include/flashvm/speech.h#L97) |
| `fvm_tts_open` | function | [include/flashvm/speech.h:103](../../../include/flashvm/speech.h#L103) |
| `fvm_tts_speak` | function | [include/flashvm/speech.h:107](../../../include/flashvm/speech.h#L107) |
| `fvm_tts_stop` | function | [include/flashvm/speech.h:111](../../../include/flashvm/speech.h#L111) |
| `fvm_tts_system_backend` | function | [include/flashvm/speech.h:87](../../../include/flashvm/speech.h#L87) |
| `fvm_tts_unix_backend` | function | [include/flashvm/speech.h:52](../../../include/flashvm/speech.h#L52) |
| `FVM_TtsBackend` | struct | [include/flashvm/speech.h:14](../../../include/flashvm/speech.h#L14) |
<!-- source-links:end -->
