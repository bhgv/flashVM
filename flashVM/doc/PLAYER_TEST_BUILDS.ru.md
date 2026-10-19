# Тестовые сборки плеера — v0.582

v0.616: общий плеер явно устанавливает AsBroadcaster для SWF6–8 до выполнения
сцены; более старые ролики пропускают установку. [Контракт EN](AS_BROADCASTER.md#transport-and-player-v0616) /
[RU](AS_BROADCASTER.ru.md#перенос-и-плеер-v0616). Нативные пересборки целевых ОС и
приёмка физического воспроизведения отделены от программных Linux CLI-fixtures.


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
перед каждым [`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L66) копирует в него снимок
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

## Звук и владение в v0.582 (изменения v0.583 ниже)

[audio.c](../frontend/common/audio.c) проверяет [`fvm_swf_sound_event_count`](../include/flashvm/swf_sound.h#L56). Для root
событий без --mute создаёт SDL device 48000 Hz stereo, configured mixer на 32
голоса и output producer block512/target2048 frames. Callback
[`fvm_movieclip_set_sound_callback`](../include/flashvm/movieclip.h#L21) доставляет ordinal до DoAction;
[`fvm_swf_sound_mixer_submit`](../include/flashvm/swf_sound_mixer.h#L50) готовит голос либо применяет stop/no-multiple.
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
Нужны C89/C++98 compiler, CMake, SDL2 >=2.0.9 CONFIG/SDL2::SDL2. Для отдельного
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

## Исправление звука Android в v0.583

Пользователь сообщил о случайных стуках/треске в Android v0.582. Подача PCM из
цикла рендеринга и фиксированный запас 2048 frames (около 43 мс) могли приводить
к опустошению очереди при задержках рендеринга или увеличенном native-блоке.
Это вероятный сценарий, а не подтверждённый на устройстве диагноз.

Теперь отдельный SDL worker подаёт PCM блоками 1024 frames. Цель очереди —
max(4800 frames, четыре фактических блока устройства); resume после двух блоков
или 1024 frames. Worker просыпается каждые 5 мс, до восьми pump за проход.
Submit, pause, stop и наблюдение очереди защищены mutex. VM/GC остаются в основном
потоке. Запас увеличивает задержку; hard realtime и коррекция дрейфа A/V не заявлены.
В --stats добавлены audio_underruns и sound_stops, в SDL log — фактические
quantum/target/resume.

StopSounds и stopAllSounds теперь очищают prepared mixer/queue через
[привязки контекста](AUDIO_STOP_BINDINGS.ru.md), даже до первой записи PCM.
Третий SWF 03-stop-sounds отменяет начальный тон и loop, затем звучат stereo/ADPCM.
В меню три SWF × два режима. Старые скачанные Linux/Windows v0.582 не обновляются
сами: их нужно пересобрать. Новый APK нужно прослушать на проблемном телефоне в
обоих режимах и проверить уход в фон/возврат. Linux dummy-тест этого не заменяет.

## v0.592 Selection test / Проверка Selection

SWF 02 now sets root focusEnabled and calls Selection.setFocus("_root") once.
Root-focused press/release counters are independent of the Key listener; each
focused release increases the blue box height. The root remains selected across
loops. The player explicitly installs the MovieClip-only namespace for SWF6–8;
automatic mouse/Tab focus and Android virtual-keyboard/IME input remain open.
[EN contract](SELECTION.md) / [RU](SELECTION.ru.md).

В SWF 02 отдельные обработчики выбранного корня и слушателя Key считают одно
нажатие/отпускание независимо. Высота синего квадрата показывает отпускания корню.
Установка Selection явная и только для SWF6–8. Это не автоматический выбор
мышью/Tab и не реализация экранной IME-клавиатуры Android. Исходные звуковые
данные и кодирование тонов сохранены.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_set_pointer` | function | [include/flashvm/input.h:72](../include/flashvm/input.h#L72) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `fvm_movieclip_set_sound_callback` | function | [include/flashvm/movieclip.h:21](../include/flashvm/movieclip.h#L21) |
| `fvm_swf_sound_event_count` | function | [include/flashvm/swf_sound.h:56](../include/flashvm/swf_sound.h#L56) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:50](../include/flashvm/swf_sound_mixer.h#L50) |
<!-- source-links:end -->
