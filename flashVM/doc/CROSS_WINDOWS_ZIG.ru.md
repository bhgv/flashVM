# Кросс-сборка Windows x64 через Zig (v0.770)

[English](CROSS_WINDOWS_ZIG.md) · [Цели Linux](CROSS_LINUX_ZIG.ru.md) · [Приёмка платформ](PLATFORM_VALIDATION.md)

[Toolchain](../cmake/toolchains/zig-windows.cmake) явно выбирает
x86_64-windows-gnu для C89/C++98. Нужна полная установка Zig для хоста сборки
с его стандартной библиотекой; путь к исполняемому файлу абсолютный. Проверяемая
конфигурация: Zig 0.13.0, CMake 4.4, хост x86-64 Linux. Другие версии и хосты
требуют своих проверок. Это Windows/MinGW, отдельная цель от Linux/musl и Android.
Прежний [MinGW toolchain](../cmake/toolchains/mingw-x64.cmake) сохранён.

Zig предоставляет компилятор, архиватор, целевые заголовки и runtime, но не SDL.
Библиотеки проекта, C++/исключений и SDL линкуются статически; системные DLL Windows,
включая UCRT API sets, остаются импортами ОС. Системный C runtime остаётся зависимостью. Нельзя выводить отсутствие всех DLL или поддержку любой
версии Windows. SDK хоста не подменяет целевой SDK.

## Минимальное ядро и консольные программы

Из каталога компонента, заменив /opt/zig/zig реальным абсолютным путём:

```sh
cmake -G "Unix Makefiles" -S . -B build-win-core \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/zig-windows.cmake \
  -DFVM_ZIG_EXECUTABLE=/opt/zig/zig \
  -DFVM_ZIG_TARGET=x86_64-windows-gnu \
  -DFVM_DEFS_FILE=cmake/profiles/minimal.defs \
  -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON \
  -DFVM_BUILD_REFERENCE_RUNNER=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-win-core --target \
  flashvm_tests_swf_stream_storage flashvm_tests_swf_stream_pcm_reader_api \
  flashvm_tests_swf_stream_adpcm_allocation flashvm_examples_multimedia_swf_stream_main \
  --parallel 3
```

Собирается всё минимальное ядро и выбранные программы, а не все опциональные
профили и зависимости. Тесты не запускаются; host process/timezone harnesses
исключаются CMake для cross. Не выдавать запуск cross CTest на хосте за приёмку.

Оба Zig-параметра обязательны и фиксируются в build-каталоге. Для смены компилятора
или цели нужен новый каталог. Относительный/отсутствующий путь к компилятору,
каталог вместо файла и неподдержанный target отклоняются. try_compile строит
статические архивы; окончательная линковка выбранных программ всё равно необходима.
Поиск библиотек, include и packages целевой; исполняемые build-tools относятся к хосту.

При переносе toolchain копировать **оба** файла: zig-windows.cmake и
[zig-windows-archives.cmake](../cmake/toolchains/zig-windows-archives.cmake).
Второй восстанавливает Zig ar/ranlib после Windows platform rules CMake, включая
временный архив длинного списка объектов GNU Make. Shell-wrapper, host GCC archiver
и фиксированный путь к репозиторию в поставляемом toolchain не требуются.
Компилятор Windows RC-ресурсов и произвольные сторонние build-системы не настраиваются.

## Целевая статическая SDL

Нужны исходники SDL **2.30.0** вне FlashVM с сохранённой лицензией upstream.
[Initial cache](../cmake/sdk/sdl2-windows-static.cmake) сохраняет Windows video,
WASAPI/WinMM и dummy audio/video, отключает SDL2main, внешний resampler и
опциональные GPU/joystick/haptic/sensor-интеграции. WinMM включается конфигурацией
SDL_AUDIO на desktop Windows; отдельной опции SDL_WMM в SDL 2.30 нет.
Плеер имеет собственный адаптер UTF-16 командной строки и программный вывод.
Для исходников ../SDL-2.30.0:

```sh
cmake -G "Unix Makefiles" -S ../SDL-2.30.0 -B build-win-sdl \
  -C "$PWD/cmake/sdk/sdl2-windows-static.cmake" \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/zig-windows.cmake" \
  -DFVM_ZIG_EXECUTABLE=/opt/zig/zig \
  -DFVM_ZIG_TARGET=x86_64-windows-gnu \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PWD/sdk-win"
cmake --build build-win-sdl --target install --parallel 3
```

## Плеер и библиотека встраивания

```sh
cmake -G "Unix Makefiles" -S . -B build-win-player \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/zig-windows.cmake \
  -DFVM_ZIG_EXECUTABLE=/opt/zig/zig \
  -DFVM_ZIG_TARGET=x86_64-windows-gnu \
  -DFVM_DEFS_FILE=cmake/profiles/player-jit.defs \
  -DSDL2_DIR="$PWD/sdk-win/lib/cmake/SDL2" \
  -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_ENABLE_SDL_AUDIO=ON \
  -DFVM_PLAYER_SOCKET_HTTP=ON -DFVM_BUILD_TESTS=OFF \
  -DFVM_BUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-win-player --target flashvm_player --parallel 3
```

Профиль включает AGG, zlib, Windows threads/OS ports, код interpreter/JIT и SDL host.
Socket HTTP использует Winsock; HTTPS требует отдельно настроенного TLS provider
и целевого SDK. flashvm_player_host здесь статический архив для C/C++ embedding:
использовать C-заголовки и транзитивные CMake-зависимости, а не CLI main в IDE.
Shared-host/DLL требует отдельной сборки и приёмки. Шрифты, image codecs, SQLite и
общие media SDK этим профилем отключены; их готовность из этой сборки не следует.

## Проверка образов без запуска

```sh
python3 scripts/check_windows_pe_inspector.py
python3 scripts/check_zig_windows_toolchain.py \
  --zig /opt/zig/zig --target x86_64-windows-gnu \
  --work /tmp/flashvm-windows-toolchain-check \
  --artifact build-win-core/flashvm_tests_swf_stream_storage.exe \
  --artifact build-win-player/frontend/flashvm_player.exe
```

Work-каталог отсутствует либо пуст. Проверка переносит оба toolchain-файла в
каталог с пробелами, строит строгий C89-архив и C++98-программу с исключением,
проверяет PE/imports, повтор configure, отклонение смены входных параметров и
отсутствия companion. Целевые программы не исполняются. Python-тест инспектора
зарегистрирован как нативный CTest flashvm_windows_pe_inspector.

[inspect_windows_pe.py](../scripts/inspect_windows_pe.py) проверяет ограниченное
подмножество: границы DOS/COFF/PE32+, x64 executable flags, отображённые секции
без перекрытия, завершённую import table и прямые имена системных DLL/API-set.
DLL, PE32, другой CPU, delay imports, некорректные диапазоны и неизвестные прямые
импорты отклоняются. Это проверка build-профиля, а не полный loader, подпись,
антивирус или security audit зависимостей. Транзитивные импорты и LoadLibrary не
анализируются; допустимое написание API-set не доказывает наличие в конкретной ОС.

```sh
python3 scripts/inspect_windows_pe.py build-win-player/frontend/flashvm_player.exe
```

## Отдельная приёмка на Windows

Перенести программы и ресурсы на Windows. Пример PowerShell из каталога компонента:

```powershell
.\build-win-player\frontend\flashvm_player.exe --version
$env:SDL_VIDEODRIVER = "dummy"
$env:SDL_AUDIODRIVER = "dummy"
.\build-win-player\frontend\flashvm_player.exe --once --stats examples/player/pcm_stream/adpcm-stream.swf
.\build-win-core\flashvm_tests_swf_stream_storage.exe
.\build-win-core\flashvm_tests_swf_stream_pcm_reader_api.exe
```

Затем с удалёнными dummy-переменными проверить реальные окно/звук, пути, HTTP,
pause/navigation, отладчик и эквивалентность interpreter/JIT. Dummy сам по себе не
подтверждает физические устройства. Cross-link/PE-инспекция не означают запуск
Windows/Wine. Все девять стадий открыты.

Первичные источники: [Zig 0.13](https://ziglang.org/download/0.13.0/release-notes.html),
[Microsoft PE/COFF](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format),
[SDL Windows](https://wiki.libsdl.org/SDL2/README-windows).

## Результат ревизии и оставшаяся приёмка

Отдельный compiler fixture, повтор configure и шесть проверок отказа прошли.
Полное минимальное ядро слинковано с четырьмя перечисленными программами. SDL
2.30.0 собрана и установлена для Windows: generated config включает Windows video,
WASAPI, WinMM и dummy audio/video. Полный player включает Windows entry, статические
host/SDL/AGG/zlib и Winsock HTTP. Fixture и пять программ проекта прошли PE32+ x64/
проверку прямых импортов. В импортах есть UCRT API sets, Kernel32, Winsock, у плеера
также GUI/COM/WinMM. SDL2.dll и отдельного C++/pthread runtime среди прямых импортов
проверенных образов нет. Ручные отложенные LoadLibrary не анализируются.

Host-тест инспектора проверяет один корректный синтетический образ и 11 независимых
некорректных/неподдержанных вариантов. Процессы Windows/Wine здесь не запускались.
Проверенный generator — Unix Makefiles; другие generators и shared-host DLL
требуют отдельной проверки. Ограниченный хост использовал незакоммиченный launcher
для определения Zig собственного пути; поставляемый toolchain использует обычный
Zig с соседней библиотекой и документированный companion без этого launcher.
