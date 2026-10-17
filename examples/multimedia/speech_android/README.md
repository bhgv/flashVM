# speech_android — Android: Context, permissions, JNI и речевые службы

Исходники интеграции **в реальное Android приложение**, не desktop executable
и не готовый APK. bridge.cpp содержит JNI host с двумя независимыми sessions,
SpeechHost.java — разрешения и Java entrypoints. Настоящие system helpers находятся
в src/multimedia/speech/tts/FlashVMTts.java и stt/FlashVMStt.java.

## Подготовка Android приложения / App preparation

1. Создайте обычный NDK/JNI host (minSdk 23+, Java 8+, main Looper). Скопируйте
   SpeechHost.java в пакет org.flashvm.example, а два system helper источника —
   в org.flashvm.multimedia. Сохраните имена или согласованно измените JNI names.
2. Подготовьте настоящий Android toolchain и соберите FlashVM для ANDROID с
   NO_MEDIA: speech не требует FFmpeg. В host CMake до add_subdirectory FlashVM
   установите CMAKE_POSITION_INDEPENDENT_CODE=ON; отключите desktop tests/examples.
   Создайте shared target `flashvm_speech_example` из bridge.cpp и свяжите flashvm.
   Host target сохраняет C++98, а JNI include предоставляет настоящий NDK.
3. В Manifest объявите RECORD_AUDIO. Для Android 11+ добавьте queries intent
   actions android.intent.action.TTS_SERVICE и android.speech.RecognitionService.
   Не добавляйте INTERNET без понимания политики выбранного system provider.
4. Установите реальные voice/recognition services и language data на устройстве.
   TTS availability не означает доступность STT. EXTRA_PREFER_OFFLINE — пожелание,
   не гарантия offline режима.
5. Сохраните packaged helper classes/methods от R8/ProGuard переименования:
   keep org.flashvm.multimedia.FlashVMTts/FlashVMStt со всеми членами и
   org.flashvm.example.SpeechHost с native entrypoints.

Пример host CMake fragment (пути заменить настоящими путями проекта):

```cmake
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Static FlashVM will enter a real JNI shared library.
set(FVM_BUILD_TESTS OFF CACHE BOOL "" FORCE) # Do not run desktop suites on the device host.
set(FVM_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # This app owns its integration target.
set(FVM_DEFS_FILE "/actual/FlashVM/cmake/profiles/minimal.defs" CACHE FILEPATH "" FORCE) # AUTO_PLATFORM resolves to ANDROID under the NDK.
add_subdirectory(/actual/FlashVM flashvm-build) # Build the actual native VM and speech code.
add_library(flashvm_speech_example SHARED /actual/examples/multimedia/speech_android/bridge.cpp) # Match System.loadLibrary.
set_target_properties(flashvm_speech_example PROPERTIES CXX_STANDARD 98 CXX_STANDARD_REQUIRED ON) # Keep first-party C++98.
target_link_libraries(flashvm_speech_example PRIVATE flashvm) # Reuse real system adapter linkage.
```

## Последовательность использования / Actual host lifecycle

Все example JNI entrypoints сериализуются на UI thread; это один teaching host,
не универсальный многопользовательский session registry. `openTts` получает
getApplicationContext(); native code берёт JavaVM и глобальные Context/class refs
из Java-originated вызова с правильным class loader. Service open идёт явно.
`SpeechHost.say` передаёт UTF-8 byte[], speak может вернуть NOT_FOUND до OnInit:
сохраните пользовательский текст и повторите позже из UI. OK означает scheduling.

Для STT сначала вызовите prepareMicrophone(Activity). Если нет grant, настоящий
requestPermissions запускает диалог и функция возвращает false: дождитесь
Activity permission result, затем только по намеренному host действию вызывайте
openStt(applicationContext). Во время listening UI timer/handler вызывает poll;
null означает пока нет текста, native failure — IllegalStateException.
`event[0]` есть final flag; transcript(event) декодирует реальные UTF-8 bytes.
После final/error не происходит автоматического повторного старта микрофона.

Всегда вызывайте SpeechHost.close на явном stop/Activity.onDestroy перед
освобождением JNI/application lifetime. Native cleanup сначала закрывает sessions,
затем host global refs; Java shutdown/cancel/destroy публикуются в main Looper.
Looper/VM должны оставаться живыми до выполнения этих операций. [GC:] Здесь нет
VM-managed edges; JNI globals и native sessions имеют самостоятельное владение.

## Проверка и ограничения / Validation and limits

JNI bridge и native Android adapter units скомпилированы в C++98 с официальным
JNI header; Java sources с Android API 35. Это compile checks, **не установленный
APK/NDK target run или проверка микрофона**. Проверьте на реальном устройстве:
нет provider; permission denied; async TTS readiness; Unicode (кириллица/emoji);
actual partial/final utterance; Activity close during service initialization.
No assertions and no fabricated recognized text are used in the example.


Исходники: [bridge.cpp](bridge.cpp), [SpeechHost.java](SpeechHost.java). Каждая непустая строка учебного C/C++/Objective-C++/Java кода
пояснена; ошибок через assertions здесь нет.

## Сборка и запуск / Build and run

Собирается внутри подготовленного native приложения соответствующей ОС согласно
описанным выше требованиям. Корневая CMake сборка не выдаёт этот файл за desktop
main или автоматически протестированный app. Нет отдельного root example target.


## Демонстрируемые API / Demonstrated API

| Function (source declarations linked below) | Detailed guide file and section |
| --- | --- |
| [`fvm_stt_android_backend`](../../../include/flashvm/speech.h#L74) | [SPEECH_BRIDGES.md — fvm_stt_android_backend](../../../doc/SPEECH_BRIDGES.md#fvm_stt_android_backend) |
| [`fvm_stt_close`](../../../include/flashvm/speech.h#L106) | [SPEECH_BRIDGES.md — fvm_stt_close](../../../doc/SPEECH_BRIDGES.md#fvm_stt_close) |
| [`fvm_stt_open`](../../../include/flashvm/speech.h#L100) | [SPEECH_BRIDGES.md — fvm_stt_open](../../../doc/SPEECH_BRIDGES.md#fvm_stt_open) |
| [`fvm_stt_poll`](../../../include/flashvm/speech.h#L102) | [SPEECH_BRIDGES.md — fvm_stt_poll](../../../doc/SPEECH_BRIDGES.md#fvm_stt_poll) |
| [`fvm_tts_android_backend`](../../../include/flashvm/speech.h#L69) | [SPEECH_BRIDGES.md — fvm_tts_android_backend](../../../doc/SPEECH_BRIDGES.md#fvm_tts_android_backend) |
| [`fvm_tts_close`](../../../include/flashvm/speech.h#L98) | [SPEECH_BRIDGES.md — fvm_tts_close](../../../doc/SPEECH_BRIDGES.md#fvm_tts_close) |
| [`fvm_tts_open`](../../../include/flashvm/speech.h#L92) | [SPEECH_BRIDGES.md — fvm_tts_open](../../../doc/SPEECH_BRIDGES.md#fvm_tts_open) |
| [`fvm_tts_speak`](../../../include/flashvm/speech.h#L94) | [SPEECH_BRIDGES.md — fvm_tts_speak](../../../doc/SPEECH_BRIDGES.md#fvm_tts_speak) |

Структуры [FVM_TtsBackend](../../../include/flashvm/speech.h#L13), [FVM_SttBackend](../../../include/flashvm/speech.h#L24), [FVM_AndroidSpeechConfig](../../../include/flashvm/speech.h#L60),
[FVM_LinuxSttConfig](../../../include/flashvm/speech.h#L49) и ownership/status contracts:
[SPEECH_BRIDGES.md](../../../doc/SPEECH_BRIDGES.md),
[public speech.h](../../../include/flashvm/speech.h).
Все примеры различают real native operations, deterministic fixtures и пока
не выполненные platform/device acceptance runs.
## Именованные Unix sockets / Named sockets

[SocketNames.java](SocketNames.java) использует настоящие Android LocalSocket.connect
с явными Namespace.ABSTRACT и FILESYSTEM; возвращённый connected socket закрывает
caller. Java abstract name не нужно начинать с NUL: Android добавляет native marker.
Это отдельный transport preparation пример, он не имитирует speech protocol/service.
Для native binary names и SSIP см. [FVM_SpeechUnixEndpoint](../../../include/flashvm/speech.h#L38) и [fvm_tts_unix_backend](../../../include/flashvm/speech.h#L47) в
[руководстве](../../../doc/SPEECH_BRIDGES.md#fvm_tts_unix_backend). Наличие socket
namespace не означает наличие Speech Dispatcher на Android.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AndroidSpeechConfig` | struct | [include/flashvm/speech.h:60](../../../include/flashvm/speech.h#L60) |
| `FVM_LinuxSttConfig` | struct | [include/flashvm/speech.h:49](../../../include/flashvm/speech.h#L49) |
| `FVM_SpeechUnixEndpoint` | struct | [include/flashvm/speech.h:38](../../../include/flashvm/speech.h#L38) |
| `fvm_stt_android_backend` | function | [include/flashvm/speech.h:74](../../../include/flashvm/speech.h#L74) |
| `fvm_stt_close` | function | [include/flashvm/speech.h:106](../../../include/flashvm/speech.h#L106) |
| `fvm_stt_open` | function | [include/flashvm/speech.h:100](../../../include/flashvm/speech.h#L100) |
| `fvm_stt_poll` | function | [include/flashvm/speech.h:102](../../../include/flashvm/speech.h#L102) |
| `FVM_SttBackend` | struct | [include/flashvm/speech.h:24](../../../include/flashvm/speech.h#L24) |
| `fvm_tts_android_backend` | function | [include/flashvm/speech.h:69](../../../include/flashvm/speech.h#L69) |
| `fvm_tts_close` | function | [include/flashvm/speech.h:98](../../../include/flashvm/speech.h#L98) |
| `fvm_tts_open` | function | [include/flashvm/speech.h:92](../../../include/flashvm/speech.h#L92) |
| `fvm_tts_speak` | function | [include/flashvm/speech.h:94](../../../include/flashvm/speech.h#L94) |
| `fvm_tts_unix_backend` | function | [include/flashvm/speech.h:47](../../../include/flashvm/speech.h#L47) |
| `FVM_TtsBackend` | struct | [include/flashvm/speech.h:13](../../../include/flashvm/speech.h#L13) |
<!-- source-links:end -->
