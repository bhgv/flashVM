# Кросс-сборка Linux/musl через Zig

[English](CROSS_LINUX_ZIG.md) · [приёмка платформ](PLATFORM_VALIDATION.ru.md)

Независимый [toolchain](../cmake/toolchains/zig-linux.cmake) явно выбирает одну цель:
`aarch64-linux-musl`, `riscv64-linux-musl` или `x86_64-linux-musl`. Используются
`zig cc`, `zig c++`, `zig ar`, `zig ranlib`: отдельный архиватор GCC и shell-обёртка
не требуются. Публичные заголовки остаются C89, реализация — C++98. Исполняемые
файлы по умолчанию линкуются статически. Для минимального профиля SDL не нужен.
Это сборка **Linux/musl**, а не Android/Bionic, Windows или macOS. Для последних
нужны собственные NDK/toolchain/SDK; см. руководство по приёмке платформ.

Установите полный дистрибутив Zig для **машины сборки**, CMake, Make либо Ninja
и Python 3 для дополнительной проверки. Укажите абсолютный путь к Zig; сохраните
его стандартную библиотеку рядом с установленным компилятором. В этой ревизии
используются Zig 0.13.0 и CMake 4.4. Другие версии требуют отдельных проверок.
При первой сборке компилируются целевые C/C++ runtime-библиотеки Zig: она может
занять больше времени. Каталоги кеша должны быть доступны для записи.
Все команды ниже выполняются **внутри компонента `flashVM/`**. Соседние проекты
компиляторов и линкера не требуются. При отдельной поставке сохраните общую MIT-лицензию.

## Полная минимальная библиотека и программы

Замените `/opt/zig/zig` настоящим абсолютным путём. Для каждой цели, установки
компилятора и набора возможностей создавайте отдельный каталог сборки.

```sh
cmake -S . -B build-arm64-musl \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/zig-linux.cmake \
  -DFVM_ZIG_EXECUTABLE=/opt/zig/zig \
  -DFVM_ZIG_TARGET=aarch64-linux-musl \
  -DFVM_DEFS_FILE="$PWD/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON \
  -DFVM_BUILD_REFERENCE_RUNNER=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm64-musl --parallel 4 --target \
  flashvm_reference_swf \
  flashvm_tests_swf_stream_output_segments \
  flashvm_tests_movieclip_playback_info \
  flashvm_examples_multimedia_swf_stream_main
```

Программы линкуются с настоящей `libflashvm.a` и встроенной статической zlib.
Это не проверки отдельных исходных файлов. `minimal.defs` отключает графику,
шрифты, кодеки изображений, SQLite и медиаадаптеры, сохраняя VM, загрузчик SWF
и системные порты. Преобразование подготовленного PCM не требует аудиоустройства.
Для RISC-V64 выберите `riscv64-linux-musl` и новый каталог `build-riscv64-musl`;
для x86-64/musl — `x86_64-linux-musl` и ещё один отдельный каталог.

Отсутствующий/относительный путь к компилятору, каталог вместо файла, неизвестная
или неуказанная цель останавливают настройку. Смена любого из двух параметров Zig
в существующем кеше также запрещена: нужен новый каталог. Ручная подмена CMake
compiler/target flags выходит за предполагаемый контракт toolchain.
Не смешивайте заголовки и библиотеки машины сборки и целевой системы. Программы
ищутся для машины сборки; заголовки, библиотеки и пакеты — в целевых корнях.
Для дополнительных SDK задавайте целевые расположения пакетов и `CMAKE_FIND_ROOT_PATH`.

## Независимая проверка toolchain

```sh
python3 scripts/check_zig_linux_toolchain.py \
  --zig /opt/zig/zig --target aarch64-linux-musl \
  --work /tmp/flashvm-arm64-toolchain-check
```

Каталог результатов должен отсутствовать либо быть пустым. `--cmake` выбирает
исполняемый файл CMake. Скрипт копирует только toolchain в отдельный путь с
пробелами, собирает строгую C89-библиотеку и C++98-программу с исключениями,
проверяет сигнатуру архива, CPU/тип ELF и таблицу сегментов. Затем проверяет
повторную настройку, отказ при смене цели/компилятора и ошибочных параметрах.
Целевая программа **не запускается**. Логи и созданные исходники остаются в
указанном каталоге, вне отслеживаемого исходного дерева. Правильный ELF-заголовок
подтверждает сборку, но не работу инструкций CPU, ОС, сети, аудио и устройств.

## Сборка и линковка SDL-плеера для ARM64

Нужен отдельный неизменённый дистрибутив SDL 2.30.0, например `/opt/src/SDL2`.
[Начальный кеш SDK](../cmake/sdk/sdl2-linux-dummy.cmake) выбирает статическую
библиотеку с dummy-аудио и dummy-видео без графических/аудиозависимостей машины
сборки. Он предназначен для проверки сборки/линковки и программного вывода,
а не физических устройств. Абсолютные пути ниже замените доступными для записи.

```sh
cmake -S /opt/src/SDL2 -B /tmp/sdl-arm64-build \
  -C "$PWD/cmake/sdk/sdl2-linux-dummy.cmake" \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/zig-linux.cmake" \
  -DFVM_ZIG_EXECUTABLE=/opt/zig/zig -DFVM_ZIG_TARGET=aarch64-linux-musl \
  -DCMAKE_INSTALL_PREFIX=/tmp/sdl-arm64-sdk -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/sdl-arm64-build --target install --parallel 4
cmake -S . -B build-player-arm64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/zig-linux.cmake \
  -DFVM_ZIG_EXECUTABLE=/opt/zig/zig -DFVM_ZIG_TARGET=aarch64-linux-musl \
  -DFVM_DEFS_FILE="$PWD/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/tmp/sdl-arm64-sdk/lib/cmake/SDL2 \
  -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_ENABLE_SDL_AUDIO=ON \
  -DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-player-arm64 --target flashvm_player --parallel 4
```

Плеер находится в `build-player-arm64/frontend/flashvm_player`. Профиль использует
встроенную статическую AGG-отрисовку, отключает шрифты/кодеки изображений/SQLite
и включает выбор JIT. Необязательные HTTP/TLS-провайдеры выключены. Исполнение
ARM64 JIT проверяется отдельно: компиляция backend не означает его исполнения.
На настоящей целевой машине такой SDK допускает `SDL_VIDEODRIVER=dummy` и
`SDL_AUDIODRIVER=dummy` с [ограниченным числом кадров плеера](../frontend/README.md).
Для физических X11/Wayland/аудиоустройств соберите отдельный SDK с заголовками,
библиотеками и драйверами целевой системы. Dummy SDK не подтверждает поддержку
физического окна/устройства. Сохраняйте upstream-лицензии SDL при поставке SDK.

## Запуск на целевой системе

Скопируйте программы и входные данные на совместимую Linux-машину. Сохраните
либо установите право исполнения и запускайте там:

```sh
./flashvm_tests_swf_stream_output_segments
./flashvm_tests_movieclip_playback_info
./flashvm_examples_multimedia_swf_stream_main converted.pcm --convert
```

Аргументы и проверки PCM описаны в [учебном примере](../examples/multimedia/swf_stream/README.md).
Reference runner требует явного SWF: [руководство](SWF_REFERENCE_RUNNER.md).
Запуск чужого бинарника на машине сборки не заменяет приёмку целевой системы.
При установленном эмуляторе можно в новом каталоге задать
`-DCMAKE_CROSSCOMPILING_EMULATOR=/absolute/path/to/qemu-aarch64`.
CTest использует адаптер для тестов, зарегистрированных как исполняемые цели.
Python-тесты, самостоятельно запускающие процессы, в cross-конфигурации FlashVM
исключены: наличие эмулятора их не включает. Без адаптера это **только сборка**;
не запускайте CTest в ожидании автоматического исполнения чужих программ.
`validate_platform.py` намеренно отвергает cross-каталоги: он предназначен
для нативной приёмки платформ.

## Подтверждения и оставшаяся работа

Проверка v0.762: полное минимальное ядро Release и четыре перечисленные программы
собраны и слинкованы для всех трёх целей. У двенадцати ELF проверены CPU,
загрузочные сегменты и отсутствие `PT_INTERP`. ARM64 SDL-плеер также статически
слинкован с целевым SDL 2.30.0 и встроенным AGG. ARM64/RISC-V64 программы
не запускались: целевые машины и QEMU здесь недоступны.
Отдельный fixture toolchain прошёл для трёх целей: пути с пробелами, запрет смены
компилятора/CPU и пять случаев ошибочных параметров.
На этой Linux-машине x86-64 действительно запущены x86-64/musl-программы:
прошли fixture PCM-сегментов и MovieClip playback-info, точный PCM и ошибки
учебного примера (включая независимую сверку FFmpeg), созданные SWF-сценарии
reference runner. Это нативное исполнение с другой libc, а не исполнение на другом CPU.
На ограниченной машине сборки применялся незакоммиченный служебный launcher
для определения Zig собственного пути. Поставляемый toolchain вызывает обычный
Zig напрямую и не устанавливает/не требует эту обёртку.
Логи, установленные SDK и кеши инструментов находятся вне исходного репозитория.

См. [состояние стадий](ROADMAP_STATUS.md). Физические окна/устройства Linux,
совместимость с эталонным Flash, остальные ОС и исполнение на ARM64/RISC-V64
требуют отдельной приёмки. Статический musl-бинарник не гарантирует поддержку
любого ядра Linux, набора расширений CPU или системной службы.

Повторяемый параметр `--artifact PATH` дополнительно проверяет уже слинкованные
программы FlashVM по тем же правилам ELF после успешной сборки отдельного fixture.
