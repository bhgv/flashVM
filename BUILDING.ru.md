# Нативная сборка и сборка для другой ОС/процессора

[English](BUILDING.md). Кросс-сборка подтверждает компиляцию/линковку, но не
исполнение на другой машине. [Приёмка](flashVM/doc/ROADMAP.md) требует реальных
запусков, устройств и проверки совместимости; этап 9 ещё открыт.

## Компоненты, независимость и языки реализации

| Компонент | Каталог для CMake -S | Основные цели | Язык |
| --- | --- | --- | --- |
| FlashVM | flashVM | flashvm, примеры встраивания | ABI C89, реализация C++98 |
| Фронтенды AS1/2/3/Limbo | compiler либо compiler/as1, as2, as3, limbo | as1_parse, as2_parse, as3_parse, limbo_parse, библиотеки C API | C99 |
| FIR и числовой адаптер | fir/v1 либо fir/v1/compiler | fir1, fir1_source, fir1_ir, fir1_source_api | C99 |
| Оптимизатор FIR | fir/v1 либо fir/v1/optimiser | fir1_opt, общий API FIR | C99 |
| Линкер SWO/SWA/ресурсов | linker | swl, swl_core | C99 |
| Общий пример IDE на C | linker/examples/ide | ide_pipeline | C99 |

Проекты имеют собственные CMake, зависимости и интерфейсы. Необязательный
[шаблон toolchain](toolchains/explicit.cmake) можно отдельно скопировать к любому
компоненту: скрытого включения соседних реализаций нет. При отделении сохраняйте
корневую MIT LICENSE и уведомления внешних библиотек.

**Хост** исполняет CMake, Python, Coco/R и форматтеры. **Цель** исполняет собранную
VM, компилятор или IDE. Coco/R обязан быть программой ХОСТА, даже если его парсер
компилируется для другой ОС/CPU. Явно задавайте абсолютный путь к хостовому
генератору; целевой cocor.exe нельзя запускать как шаг сборки на Linux.
[Внешний проект генератора](https://github.com/bhgv/coco-r-C) сюда не копируется.

Сборка fir1 для ARM не превращает FIR в машинный код ARM: текущий backend
выдаёт переносимые AVM1-объекты SWO, а SWL собирает SWF8. Архитектура программы
компилятора и формат выдаваемого Flash-байткода — разные параметры. Нативного
LLVM backend FIR пока нет.

## Сборка и тесты на текущей машине

Нужны компиляторы C/C++, CMake 3.16+ для компонентов C99, Python 3.9+ для тестов,
внешний Coco/R для хоста. VM допускает CMake 3.10+, её отчёт нативной приёмки
требует CMake/CTest 3.26+. Команды ниже рассчитаны на одноконфигурационный
генератор. Для Visual Studio/Xcode добавляйте --config Release и ctest -C Release;
исполняемые файлы могут находиться в Release/ и иметь суффикс .exe.

Из корня репозитория:

```sh
COCO_HOST=/absolute/path/to/coco-r-C
COCO_EXE="$COCO_HOST/sources/cocor"
cmake -S compiler -B build/native-compiler \
  -DCOCOR_ROOT="$COCO_HOST" -DCOCOR_EXECUTABLE="$COCO_EXE"
cmake --build build/native-compiler --parallel 2
ctest --test-dir build/native-compiler --output-on-failure

cmake -S fir/v1 -B build/native-fir \
  -DCOCOR_ROOT="$COCO_HOST" -DCOCOR_EXECUTABLE="$COCO_EXE"
cmake --build build/native-fir --parallel 2
ctest --test-dir build/native-fir --output-on-failure

cmake -S linker -B build/native-linker
cmake --build build/native-linker --parallel 2
ctest --test-dir build/native-linker --output-on-failure

cmake -S flashVM -B build/native-vm -DCMAKE_BUILD_TYPE=Release \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON
cmake --build build/native-vm --parallel 2
ctest --test-dir build/native-vm --output-on-failure
```

Минимальный профиль VM отключает растеризатор, шрифты, image codecs и SQLite,
сохраняет нативные порты и инфраструктуру render jobs. Профили agg.defs/cairo.defs
включают соответствующие статические библиотеки; их работа проверяется отдельно.
C99-проектам VM не требуется. Если стандартная временная папка недоступна, задайте
TMPDIR на доступный для записи каталог.

## Другой процессор Linux

Нужен полный SDK цели: компилятор, assembler/linker, libc и C++ runtime для VM.
Для каждой ОС/CPU/ABI/набора инструментов создавайте отдельную папку сборки.
Не заменяйте компилятор в старом CMake cache. Шаблон сохраняет SDK-параметры в
проверках компилятора, не отключает настоящую пробную линковку и не ищет хостовые
программы внутри sysroot.

```sh
cmake -S fir/v1 -B build/linux-arm64-fir \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/toolchains/explicit.cmake" \
  -DFVM_TARGET_SYSTEM=Linux -DFVM_TARGET_PROCESSOR=aarch64 \
  -DFVM_TARGET_CC=/sdk/bin/aarch64-linux-gnu-gcc \
  -DFVM_TARGET_AR=/sdk/bin/aarch64-linux-gnu-ar \
  -DFVM_TARGET_RANLIB=/sdk/bin/aarch64-linux-gnu-ranlib \
  -DCOCOR_ROOT="$COCO_HOST" -DCOCOR_EXECUTABLE="$COCO_EXE" \
  -DBUILD_TESTING=OFF
cmake --build build/linux-arm64-fir --parallel 2
```

Для всех фронтендов замените -S на compiler; для линкера — linker и уберите Coco/R.
Для VM укажите -S flashVM, дополнительно
-DFVM_TARGET_CXX=/sdk/bin/aarch64-linux-gnu-g++, абсолютный путь minimal.defs,
-DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=OFF. FVM_TARGET_SYSROOT необязателен,
если компилятор уже знает свой SDK; не задавайте хостовый /usr вместо целевого SDK.
Для RISC-V64 берите согласованные riscv64-linux-gnu инструменты и processor riscv64;
для ARM32/x86 — соответствующий SDK/ABI. ISA, float ABI и минимальная версия ОС
должны соответствовать устройству. Одного названия CPU недостаточно для выбора ABI.
Компилятор/wrapper обязан уже выбирать свою цель: шаблон не выдумывает -march.

## Windows из другой ОС

Используйте Windows SDK/MinGW с согласованными C/C++ runtime. Существующий
[MinGW x64 toolchain VM](flashVM/cmake/toolchains/mingw-x64.cmake) сохранён;
общий явный шаблон подходит всем компонентам:

```sh
cmake -S linker -B build/windows-x64-linker \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/toolchains/explicit.cmake" \
  -DFVM_TARGET_SYSTEM=Windows -DFVM_TARGET_PROCESSOR=AMD64 \
  -DFVM_TARGET_CC=/sdk/bin/x86_64-w64-mingw32-gcc \
  -DFVM_TARGET_CXX=/sdk/bin/x86_64-w64-mingw32-g++ \
  -DFVM_TARGET_RC=/sdk/bin/x86_64-w64-mingw32-windres \
  -DBUILD_TESTING=OFF
cmake --build build/windows-x64-linker --parallel 2
```

Подстановка compiler/FIR и хостовый Coco/R — как выше. Windows .exe не является
Linux-программой. Wine/эмулятор — отдельное свидетельство, не нативная приёмка
Windows. Проверяйте реальные DLL-зависимости: статический архив библиотеки ещё
не означает статический runtime всего приложения.

## Android: другая ОС и CPU через NDK

Используйте родной toolchain NDK с конкретными API/ABI, а не общий шаблон.
Генератор Coco/R, Python и CMake по-прежнему работают на хосте:

```sh
cmake -S fir/v1 -B build/android-arm64-fir \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
  -DCOCOR_ROOT="$COCO_HOST" -DCOCOR_EXECUTABLE="$COCO_EXE" \
  -DBUILD_TESTING=OFF
cmake --build build/android-arm64-fir --parallel 2
```

Для compiler/linker замените исходный каталог; VM также требует профиль minimal
и собственные FVM_BUILD_* переключатели. Каждый NDK ABI — отдельная build-папка.
CLI надо разместить в подходящем исполняемом каталоге устройства; APK требует
нативного хоста/activity и упаковки, не только статической библиотеки.
[Android frontend](flashVM/frontend/android/README.md) и
[device adapter](flashVM/scripts/android_test.py) описывают отдельный player/CTest
путь. Новая проверка Android SDK/устройства в этой серии не заявляется.

На Android/Termux нативная сборка использует обычные команды CMake/Clang,
но Coco/R тоже должен быть собран для Android CPU. Desktop-генератор на телефоне
не запустится. Патчи можно хранить в downloads; компилировать/исполнять лучше
в файловой системе приложения Termux. Наличие каждого SDK в конкретном пакете
Termux этим руководством не обещается.

## macOS и выбор Apple CPU

На macOS используйте Xcode/Command Line Tools SDK и нативные команды. Для другого
поддерживаемого Apple CPU добавьте -DCMAKE_OSX_ARCHITECTURES=arm64 либо x86_64 в
новую папку. Universal arm64;x86_64 требует обеих SDK/library slices. Coco/R
исполняется на CPU хоста. Подпись, JIT entitlements/MAP_JIT и реальное исполнение
проверяются отдельно. Для кросс-сборки из Linux нужен законно предоставленный
Apple SDK и совместимый Darwin linker; репозиторий SDK не подменяет. Новая
нативная/cross приёмка macOS здесь не заявляется.

## Исполнение тестов после кросс-сборки

BUILD_TESTING=OFF относится к C99-проектам, VM использует FVM_BUILD_TESTS=OFF.
Это намеренно: Python-тесты запускают бинарники инструментов и не должны
исполнять чужую архитектуру на хосте. Отключённые тесты не считаются прошедшими.
CTest добавляет CMAKE_CROSSCOMPILING_EMULATOR прямым тестовым executable targets,
но не произвольным программам, запускаемым внутри Python. Полную нативную
проверку инструментов выполняйте на цели с её Python/CMake/Coco/R; для VM
используйте явные описанные SDK/device адаптеры. Копия build-папки с абсолютными
путями хоста не превращается в нативную приёмку.

Проверьте архитектуру (readelf -h/PE tools), слинкуйте реального клиента, затем
запустите на нужной машине. Отчёт нативной приёмки намеренно отвергает cross:
[доказательства](flashVM/doc/RELEASE_EVIDENCE.ru.md).
[Проверенные сборки и ограничения SDK](toolchains/VALIDATION.ru.md).

[Каталог компиляторов](compiler/examples.ru.md),
[примеры FIR](fir/v1/examples/README.ru.md),
[C-пример линкера](linker/examples/ide/README.ru.md),
[примеры VM](flashVM/examples/README.md), [стиль ATG](tools/README.ru.md).

## Контракт представления C99

Фронтенды и SWL требуют 8-битный char и int не уже 32 бит: UTF-8, предел входа
2 MiB и октетные wire formats не допускают молчаливой замены на 16-битные символы.
FIR дополнительно требует 8-байтовый binary64 double (основание 2, 53 бита мантиссы,
экспонента 1024). Известный смешанный порядок слов float/integer отвергается,
когда компилятор публикует соответствующие макросы. Эти проверки не заменяют
проверку IEEE-представления неизвестной экзотической платформы. Ошибка видна
при компиляции публичного заголовка, до возможного повреждения SWO.
Пять групп [тестов заголовков](tools/test_portable_headers.py) выполняют 17
compile-only проб настоящим GCC/Clang хоста: обычный совместный C99 include и
отклонение намеренно несовместимых limit/float макросов. Это fixtures условий,
а не заявление о реальном запуске на 16-битном CPU.

```sh
python3 tools/test_portable_headers.py --cc /host/path/cc
```

Шаблон явно запрещает fallback на host C++: если проект включает C++, но
FVM_TARGET_CXX не задан, CMake прекращает конфигурацию с FVM_TARGET_CXX_REQUIRED.
Для C99-only компонентов отсутствие C++ допустимо. Четыре настоящие configure
пробы проверяют этот контракт: [test_toolchain_contract.py](tools/test_toolchain_contract.py).

## Структурный FIR-парсер и пять C API

FIR-парсер отдельно: `cmake -S fir/v1/frontend -B build-fir-syntax` с теми же
явными COCOR_ROOT/COCOR_EXECUTABLE. Цели: firsyn_parse и firsyn_frontend.
У полного fir/v1 режим `fir1 syntax` включён по умолчанию; отключение —
FIR_BUILD_SYNTAX_FRONTEND=OFF. Для всех пяти API в compiler передайте явный
FIR_SYNTAX_ROOT=/absolute/path/to/fir/v1/frontend; [пример](compiler/PARSER_COVERAGE.ru.md).
Те же параметры CMake toolchain/system/processor/compiler из разделов выше
применяются к этому C99-проекту. Coco/R остаётся HOST-инструментом. Библиотеки
и CLI должны быть собраны для целевой платформы, а не скопированы из host-build.
При отделении проекта сохраните общую MIT LICENSE и уведомления generated frames.

[0.6.0: Unicode, regexp, CONFIG, Vector, generator boundary](compiler/LEXICAL_PROFILE.ru.md).
