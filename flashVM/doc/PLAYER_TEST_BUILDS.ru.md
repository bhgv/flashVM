# Тестовые сборки плеера — v0.582

[English](PLAYER_TEST_BUILDS.md). Подготовлены Linux x64/Windows x64 пакеты и
отдельный Android ARM64 APK, [два исполняемых SWF8](../examples/player/animation_audio/README.md).
Это тест поддерживаемого подмножества, не полный Flash Player и не закрытие стадии 9.

## Запуск и проверка

Linux: распаковать архив, дать executable permission файлам flashvm_player,
run-interpreter.sh, run-jit.sh и запустить нужный скрипт. Он переходит в свою папку,
добавляет --stats и выбирает интерактивный SWF, если свой путь не передан.
В lib/ лежит SDL2. Сборка Ubuntu 24.04 x86-64, glibc 2.39; compiler runtimes
статические. Системные зависимости SDL window/audio остаются необходимыми;
совместимость со старыми glibc не обещается. Без endpoint звук завершается ошибкой;
для просмотра без устройства используйте --mute.

Windows: flashvm_player.exe, SDL2.dll, libwinpthread-1.dll должны оставаться рядом.
Двойной клик run-interpreter.bat/run-jit.bat; консоль не исчезает после ошибки.
Это PE32+ x64 console app с настоящим SDL window и существующим UTF-16→UTF-8
адаптером путей. Собрано MinGW GCC13 POSIX; imports проверены, запуск на настоящем
Windows здесь не выполнен. Команды ручного запуска: [README тестов](../examples/player/animation_audio/README.md).

Android: установить test-signed APK, при запросе ОС разрешить установку приложению,
открывающему скачанный файл. Выбрать один из четырёх пунктов: два SWF × Interpreter/
JIT. Mute отключает устройство. Back завершает процесс теста и возвращает меню.
Встроенные SWF копируются в приватную папку приложения. Это пока не файловый
браузер/SAF/import intent для любых SWF. Storage/microphone/network permissions нет.
Основное касание заменяет мышь; стрелки/Space требуют физическую/BT/USB клавиатуру.
Экранная клавиатура и распознавание мультитач-жестов не реализованы.

APK: Android 6/API23+, arm64-v8a. Проверены ELF ABI, 16 KiB PT_LOAD alignment,
zipalign для uncompressed native libs и подпись apksigner. Подпись временная
тестовая: иной signer может потребовать удаления прошлой тестовой установки.
Запуска APK на телефоне/эмуляторе здесь не было; сборка/подпись не являются
нативной приёмкой. Меню и библиотека SDL2/Java упакованы вместе.

Один SWF зациклен каждые 5 секунд. Escape/закрытие окна завершает desktop playlist;
--once/--frames ограничивают запуск. Resize сохраняет пропорции. --help перечисляет
--quality, --fps, --speed, --mute и режимы. Описание цветов/клавиш, фаз звука,
PCM8/PCM16/ADPCM и ограничения фигур: [учебник](../examples/player/animation_audio/README.md).

## HostInput

[player.c](../frontend/common/player.c) создаёт и закрепляет global HostInput;
перед каждым [`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L54) копирует в него снимок
[SDL state](../frontend/common/sdl_window.c). Это расширение плеера, не полная
реализация стандартных Flash Key/Mouse/broadcasters или button handlers.
Все VM/input/render операции сериализованы SDL-потоком приложения. Root/Input
закреплены GC pin до cleanup; native snapshots/PCM не входят в GC.

| Поле | Смысл |
| --- | --- |
| x, y | Последний основной указатель в authored world/stage pixels с учётом origin. SDL отображает координаты окна через logical viewport; в letterbox возможны координаты за сценой. Также передаются в [`fvm_input_set_pointer`](../include/flashvm/input.h#L72) для _xmouse/_ymouse. |
| left, right, up, down, space | Удержание стрелок/Space, числа 0/1. |
| buttons | Удержание основной/средней/второй кнопки, биты 1/2/4. Глобального эксклюзивного OS capture нет. |
| keyCode | Последний transition: ASCII (буквы uppercase), стрелки 37..40; остальные не-ASCII клавиши дают 0. |
| keyDown | Состояние последнего key transition; это не полный массив удержания всех клавиш. |
| keyPresses, clicks | Накопленные физические down transitions; key repeat не увеличивает keyPresses. Unsigned host counters могут переполниться; история событий не хранится. |

Focus loss сбрасывает удержание, сохраняя позицию/код/счётчики. Android background
останавливает timeline/PCM и удержание; возврат перебазирует video deadline без
catch-up burst. Activity/Surface/native thread принадлежат SDLActivity.
Короткий down/up между кадрами может проявиться только в счётчике; callbacks
пользовательских обработчиков этот snapshot-механизм не вызывает.

## Звук и владение

[audio.c](../frontend/common/audio.c) проверяет [`fvm_swf_sound_event_count`](../include/flashvm/swf_sound.h#L56). Для root
событий без --mute создаёт SDL device 48000 Hz stereo, configured mixer на 32
голоса и output producer block512/target2048 frames. Callback
[`fvm_movieclip_set_sound_callback`](../include/flashvm/movieclip.h#L21) доставляет ordinal до DoAction;
[`fvm_swf_sound_mixer_submit`](../include/flashvm/swf_sound_mixer.h#L48) готовит голос либо применяет stop/no-multiple.
Ошибки декодирования/конверсии/устройства видимы и завершают воспроизведение.
На итерацию не более четырёх pump, ожидание при наличии audio ограничено 5 ms.
[Контракт очереди](SWF_SOUND_OUTPUT.ru.md). Это не realtime callback.

Проверяются root StartSound, loops/no-multiple/stop по ID, PCM8/PCM16/ADPCM.
Wall clock видео и device clock независимы: --speed/--fps меняют интервалы событий,
не высоту звука. Есть очередь/задержка железа; drift-corrected A/V sync не обещается.
MP3/AAC/stream/sprite sound/AS Sound и подключение AVM1 StopSounds к этому отдельному
player mixer остаются открыты. Context-channel StopSounds API не менялся.
Callback снимается до освобождения userdata. Producer stop/close, mixer/device
close выполняются до SDL_Quit, в том числе после частичной ошибки инициализации.

## Повторяемая сборка

Точные desktop команды: [EN раздел](PLAYER_TEST_BUILDS.md#reproduce-desktop-builds).
Профиль [player-jit.defs](../cmake/profiles/player-jit.defs) включает AGG/JIT и
исключает fonts/image codecs/FFmpeg/SQLite; SDK этих подсистем пакету не нужны.
Нужны C89/C++98 compiler, CMake, SDL2 >=2.0.5 CONFIG/SDL2::SDL2. Для отдельного
компонента использовать -S . и его абсолютный путь профиля; compiler/linker соседей
не требуется. Linux ищет packaged SDL2 по $ORIGIN/lib, затем обычными путями ОС.

Windows cross toolchain: [mingw-x64.cmake](../cmake/toolchains/mingw-x64.cmake),
MinGW POSIX на PATH и соответствующий SDL2 SDK. libwinpthread-1.dll берётся из того
же toolchain. Для строгого C89/LLP64 zlib использует поддерживаемую upstream
32-bit CRC braid без правки внешних исходников. Перед SAPI включается windows.h.
[Native Windows](../frontend/windows/README.md).
[Android SDK/NDK и сборка APK](../frontend/android/README.md).

## Приёмка

FVM_DESKTOP_WINDOW_TESTS включает CLI/geometry/pixel/close проверки.
FVM_DESKTOP_DEMO_TESTS дополнительно требует Linux X11, SDL audio, Python3/Pillow,
xdotool и существующий DISPLAY (Xvfb допустим с явным указанием). Тест вводит
стрелку/мышь/Space, проверяет именно изменившиеся пиксели/позиции и native/interpreter/
sound-event counters обоих SWF в обоих режимах. Dummy audio подтверждает протокол,
не слышимость на физическом endpoint. Кросс-компиляция не выдаётся за запуск на
Windows/Android. [Текущие результаты](ROADMAP_STATUS.md); стадии остаются открыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_set_pointer` | function | [include/flashvm/input.h:72](../include/flashvm/input.h#L72) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:54](../include/flashvm/movieclip.h#L54) |
| `fvm_movieclip_set_sound_callback` | function | [include/flashvm/movieclip.h:21](../include/flashvm/movieclip.h#L21) |
| `fvm_swf_sound_event_count` | function | [include/flashvm/swf_sound.h:56](../include/flashvm/swf_sound.h#L56) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:48](../include/flashvm/swf_sound_mixer.h#L48) |
<!-- source-links:end -->
