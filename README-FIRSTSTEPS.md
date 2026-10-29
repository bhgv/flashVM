# Первые шаги: сборка и запуск

[English](README-FIRSTSTEPS.en.md) · [Обзор](README.ru.md) ·
[Подробная сборка и кросс-компиляция](BUILDING.ru.md)

## Обычный запуск без ключей

```sh
python3 build_menu.py
```

Введите `all`, остальные вопросы можно подтвердить Enter. Меню собирает оконный
FlashVM, инструменты и экспериментальный Linux AMD64 Dis-runtime, затем
устанавливает пакеты в `out/`. SDL2 и генераторы собираются автоматически из
закреплённых исходников. После сборки: **1** — проверки, **2** — окно FlashVM,
**4** — программа Dis, **5** — терминальный редактор, **6** — сервисы IDE,
**8** — новый MovieClip-стол, **9** — прежний одиночный графический редактор,
**10/11** — примеры владельцев и Unicode-выделения, **12** — сессионный Copy/Cut/Paste.
[Жесты и оформление стола](ide/DESKTOP-WORKSPACE.ru.md): готовая картинка при движении, два/три контакта, Ctrl+колесо над шапкой, миниатюры и Help.
[Все точки запуска](ide/ENTRYPOINTS.md). Для прямого запуска после сборки:
`python3 build-menu.py --projects disvm-editor --backend cmake --launch desktop`.
В редакторе `inspect PATH` показывает размер и hex-заголовок бинарного ресурса;
`complete` и `hint` дают AS/Limbo-дополнение и справку. См.
[редактор](ide/dis/examples/editor-console/README.md) и
[реальный Dis-runtime и границы платформ](disVM/runtime/README.md).
В новом столе работают отдельные MovieClip-окна и владельцы Limbo, миниатюры,
мышиная каретка/выделение и F7/F8. Подключённые Run/Debug, системный clipboard и часть Sys/Styx
ещё интегрируются. Ниже также описаны отдельные консольные/reference-режимы.

## Установка локальных пакетов после сборки

По умолчанию успешная сборка копирует продукты в `out/<компонент>/`: исполняемые
файлы, нужные SDK-заголовки/библиотеки, лицензии, примеры и README-FIRSTSTEPS.
`build_menu.py` и `build_workspace.sh` — совместимые имена тех же launcher-ов.

```sh
# Выбрать компоненты и движок; установка выполняется автоматически.
python3 build_menu.py --projects vm,compilers,fir,linker,symbols,ide --backend auto
# Минимальный и AGG варианты не смешиваются в одном пакете.
./build_workspace.sh --projects vm --vm-profiles minimal,agg
# Явный каталог пакетов, включая путь с пробелами.
./build_workspace.sh --projects linker --out "$PWD/out packages"
# Отключать локальную установку нужно явно.
./build_workspace.sh --projects linker --no-install
```

VM-пакеты: `out/flashvm/no-graphics-headless` и `out/flashvm/agg-headless`.
`flashvm` там — настоящий консольный runner, также сохранённый под исходным
именем flashvm_reference_swf. Он исполняет numeric SWF примеры, но не создаёт окно.
Окно, мышь/клавиатура и звук используют native SDL2 player. Отсутствующий SDK
теперь собирается автоматически из закреплённых исходников:

```sh
./build_workspace.sh --projects vm --vm-profiles agg --player
cd out/flashvm/agg-sdl2
./flashvm_player examples/player/animation_audio/02-input-audio.swf
```

При отсутствии подходящего SDK SDL2 скачивается и собирается автоматически.
`--vm-profiles default,minimal,agg,cairo` выбирает отдельные конфигурации;
доступность Cairo/SDL/прочих SDK проверяет соответствующая сборка. Примеры
animation_audio содержат графику, pointer/keyboard input и звук. Video-only и
video-audio — реальные SWF6 compatibility fixtures, но SWF video decoder/player
в FlashVM пока отсутствует; наличие файла не означает работающий показ видео.

```sh
out/flashvm/no-graphics-headless/flashvm out/flashvm/no-graphics-headless/constant.swf
out/flashvm/agg-headless/flashvm out/flashvm/agg-headless/arithmetic.swf
# Ожидаемые trace: 123 и 42.
```

В `out/ide/modules` лежат idecore/uibridge/ideinput/idebridge/idecomplete/idebuiltins
и их `.sbl`, рядом — Limbo-интерфейсы, AS worker/view и исходники примеров.
`out/ide/vm/STATUS.txt` сообщает реальный статус runner: embedded Dis-хост ещё
не закончен. При явном EMU_EXECUTABLE копируется именно этот reference executable;
его namespace и ABI-совместимый compiler/header набор по-прежнему нужны.
Один FlashVM SWF runner не исполняет idecore.dis. Подробности:
[редактор/каналы](ide/EDITOR-CHANNELS.ru.md), [AS](ide/as/README.md),
[Dis](ide/dis/bridge/README.md), [учебный tool SDK](examples/firststeps/tools/README.md).


Запускайте команды из корня репозитория. Общая сборка необязательна:
каждый проект сохраняет собственный CMakeLists.txt и независимые зависимости.
Скрипт рассчитан на нативную сборку Bash + CMake 3.10+, C99/C++ компилятором,
Make или Ninja и Python 3 для тестов. На Termux подготовьте инструменты:

```bash
pkg install git clang make cmake python
```

Физический Android этой серией не проверялся. Все внешние исполняемые файлы
должны соответствовать вашей ОС и архитектуре; Linux amd64 SDK нельзя
использовать как Android arm64 SDK.

## Меню выбора компонентов

```bash
python3 build-menu.py
```

Введите один номер (например, `4` — линкер), несколько (`1,4,6`) или имена
(`vm linker ide`). `all` выбирает всё, `q` завершает меню. Затем можно включить
тесты и консольный пример IDE. Скрипт сразу запускает выбранную сборку,
передаёт её вывод в терминал и возвращает её код завершения.
Каждый пункт — отдельный компонент; `compilers` собирает четыре фронтенда вместе.

Без интерактивных вопросов:

```bash
python3 build-menu.py --projects linker --tests
python3 build-menu.py --projects vm,linker --jobs 4
python3 build-menu.py --projects ide --demo
python3 build-menu.py --projects all --build-root build/release
# Только показать команду, ничего не собирать:
python3 build-menu.py --projects 1,4 --dry-run
```

Меню использует тот же `build-workspace.sh` и те же переменные SDK, описанные
ниже. Python — инструмент сборки на хосте; для исполнения Limbo-ядра он не нужен.

## Локальные инструменты из ваших исходников

Готовые внешние Coco/R и Limbo SDK больше не обязательны. Нужные выборки
расположены в tools/coco-r-C, tools/coco-r-limbo и tools/limbo-bootstrap.
Dis interpreter/JIT перенесён отдельно в disVM/. Источники, commit, лицензии и
обновление описаны в [tools/UPSTREAMS.ru.md](tools/UPSTREAMS.ru.md).

```bash
# Обычная сборка сама сначала соберёт нужные dependency tools из tools/.
./build-workspace.sh
# Отдельные инструменты:
./build-workspace.sh --projects cocor,cocor-limbo,limbo-bootstrap --tests
# Настоящее ядро interpreter/JIT; полный host runner ещё в работе.
./build-workspace.sh --projects disvm --tests
```

Native Limbo — временный source-built bootstrap для сложного кода IDE, а не
готовый собственный компилятор. Приоритет — наш Coco/R Limbo/FIR, debug info и
внешние модули C/AS по `.m`; см. [контракт](compiler/limbo/OWN_COMPILER.ru.md).
Обе upstream-версии Limbo, C и Limbo, сохранены рядом с bootstrap для переноса.

При необходимости отдельного совместимого SDK можно переопределить
COCOR_ROOT+COCOR_EXECUTABLE вместе, LIMBO_EXECUTABLE и LIMBO_MODULE_DIR.
Независимые/отделённые проекты по-прежнему принимают явные SDK-пути.
Для тестов **исполнения** IDE и запуска примера нужен явный совместимый
EMU_EXECUTABLE; компиляция и проверки её продуктов выполняются без него.
Архив libdisvm_core.a ещё не заменяет исполняемый эмулятор; compiler/runtime из
разных forks нельзя автоматически считать ABI-совместимыми.

## Собрать и регулярно пересобирать

```bash
./build-workspace.sh --help
./build-workspace.sh --jobs 2
# Повторите ту же команду после обновления исходников: сборка инкрементальная.
./build-workspace.sh --projects compilers,fir,linker,symbols,disvm --jobs 2 --tests
```

По умолчанию собираются FlashVM, Dis interpreter/JIT core, четыре фронтенда,
FIR-инструменты, SWF-линкер, SBL reader и Limbo-ядро IDE. SDK проверяются до начала сборки. Первая ошибка
останавливает работу; частичные результаты сохраняются для диагностики.
Скрипт устанавливает только локальные пакеты в out/ (или --out), не выполняет
системную установку, удаление пользовательских файлов, коммиты или запуск по расписанию.

```bash
# Только VM и линкер: Coco/R и Limbo не нужны.
./build-workspace.sh --projects vm,linker
# Только компиляторы и оптимизатор.
./build-workspace.sh --projects compilers,fir
# Отдельный каталог и конфигурация.
./build-workspace.sh --projects linker --build-root build/debug --config Debug --tests
```

Временные файлы находятся в build-root/tmp. Параметры SDK можно хранить
в своём shell-скрипте окружения. После смены SDK/компилятора используйте новый
`--build-root`, чтобы не смешивать несовместимые CMake-кэши.

| Результат (обычный Unix Makefiles/Ninja) | Назначение |
| --- | --- |
| build/workspace/vm/flashvm_reference_swf | Консольное исполнение SWF и trace |
| build/workspace/compilers/as1/frontend/as1_parse | Распознавание синтаксиса AS1; аналоги в as2, as3, limbo |
| build/workspace/fir/fir1_source | Ограниченное по покрытию понижение AS/Limbo в FIR |
| build/workspace/fir/fir1 | Проверка FIR и генерация SWO |
| build/workspace/fir/fir1_opt | Явная оптимизация FIR |
| build/workspace/linker/swl | SWO/SWA → SWF8 |
| build/workspace/symbols/sblmap | Проверка/чтение Limbo SBL |
| build/workspace/disvm/libdisvm_core.a | Dis/JIT core, ещё без standalone host runner |
| build/workspace/limbo-bootstrap/limbo-bootstrap | Временный локальный native compiler |
| build/workspace/ide/idecore.dis, idecore.sbl | Ядро IDE и отладочные данные |

Многоконфигурационные генераторы могут добавлять подкаталог Release/Debug.
Синтаксический разбор не означает полную реализацию языка: см.
[покрытие компиляторов](compiler/PARSER_COVERAGE.md).

## Запустить доступную часть IDE

```bash
./build-workspace.sh --projects ide --demo
```

При заданном совместимом EMU_EXECUTABLE это собирает Limbo-ядро и запускает комментированный
[пример навигации](ide/dis/examples/navigation/README.ru.md) в двух режимах
внешнего reference emu. Ожидается `PASS idecore navigation 16 checks`.
Пример использует типизированные каналы: открывает документ, ищет текст,
редактирует его и отклоняет устаревшую ревизию. `idecore.dis` — библиотечный
модуль, его нельзя запускать как самостоятельное приложение с init.

Полноценное графическое IDE и встроенный Dis/JIT ещё разрабатываются.
Данный запуск проверяет доступное ядро и клиентский пример, а не готовый
графический редактор и не доказательство нового нативного JIT.
Для разработки смотрите [протокол](ide/dis/PROTOCOL.md),
[архитектуру](ide/ARCHITECTURE.ru.md) и [отладку](ide/DIS_ACCEPTANCE.md).

## Прогнать исходник через инструменты

После сборки fir,linker,vm:

```bash
# Явный writable temp для ручных запусков, включая среды без /tmp.
export TMPDIR="$PWD/build/workspace/tmp"
mkdir -p "$TMPDIR"
build/workspace/fir/fir1_source as2 --input fir/v1/examples/as2/variables.as build/workspace/variables.fir --module variables
build/workspace/fir/fir1 check build/workspace/variables.fir
build/workspace/fir/fir1_opt build/workspace/variables.fir build/workspace/variables.opt.fir
build/workspace/fir/fir1 object build/workspace/variables.opt.fir build/workspace/variables.swo
build/workspace/linker/swl archive build/workspace/library.swa build/workspace/variables.swo
build/workspace/linker/swl link build/workspace/variables.swf build/workspace/library.swa
build/workspace/vm/flashvm_reference_swf build/workspace/variables.swf
```

Ожидаемые trace: 36, 42. [Подробности конвейера](fir/v1/doc/PIPELINE.md),
[примеры линкера](linker/examples/README.md),
[host-инструменты IDE](ide/README.ru.md).
Для окна проигрывателя отдельно соберите SDL2 frontend по
[руководству по запуску](flashVM/frontend/RUNNING.md); SDK собирается автоматически.

```bash
# Один фронтенд или несколько отдельных языков:
python3 build-menu.py --projects as1
python3 build-menu.py --projects as2,limbo
```

`all` включает группу `compilers`; отдельные языки повторно не собираются.

## История правок и точные ошибки компилятора

```bash
python3 build-menu.py --projects ide --tests --demo history
build/workspace/fir/fir1_diagnostic_example
python3 ide/build.py ide/examples/numeric/program.as build/workspace/demo.swf --language as2 --compiler build/workspace/fir/fir1_source --ir build/workspace/fir/fir1 --linker build/workspace/linker/swl --structured-diagnostics
```

Первый запуск требует Limbo/emu/module SDK и показывает Undo/Redo через каналы.
Второй — комментированный C99-клиент ожидаемой ошибки (нужна сборка fir).
Третий публикует SWF только после успешной цепочки компиляции и возвращает JSON;
ошибки исходника получают проверенные байтовые/символьные диапазоны и SHA-256
снимка. Это host-инструмент; Python не входит в runtime IDE.
[История](ide/dis/HISTORY.ru.md) ·
[Схема ошибок](fir/v1/doc/SOURCE_DIAGNOSTICS.ru.md).

## Внешний AS-модуль для Dis и AS-клиент Dis по одному .m

Собственный генератор находится в build/workspace/limbo/frontend/limbo_bindings
после `./build-workspace.sh --projects limbo`.
`--format as-module` выдаёт адаптер реализации внешнего не-Dis AS-модуля,
`--format as-proxy` — AS-клиент Dis-экспортов из того же заголовка.
Есть профили as1/as2/as3, C99, Python и JSON. Имена/типы/ID не поддерживаются вручную.
[Подробный двусторонний пример](compiler/limbo/examples/external-modules/README.ru.md).
Native transport/loader/type marshaler ещё в работе; generation не означает
готовое cross-VM исполнение.

Собственный `fir1_source ... -g DEBUG.json` выдаёт FIRDBG1 с исходником, SSA origins,
уникальными lexical scopes и переменными до оптимизации.
[Единицы и границы debug info](fir/v1/doc/SOURCE_DEBUG.ru.md).

## ReplaceAll — IDE 0.5.0

```sh
python3 build-menu.py --projects ide --demo replaceall
```

[Example / Пример](ide/dis/examples/replaceall/README.ru.md).
[Interface migration / Обновление интерфейса](ide/dis/REPLACE_ALL.ru.md): rebuild the module and every client with the same idecore.m.

[Real FIR diagnostics in Limbo IDE / Диагностика FIR в Limbo IDE](ide/dis/examples/compiler-location/README.ru.md): `./build-workspace.sh --projects fir,ide --tests` with a matching Limbo/module/emu SDK.

## CMake 3.10.2 / Make / Inferno mk

```sh
# Полная сборка из исходников с CMake 3.10.2:
./build-workspace.sh
# Генератор yacc, временный Limbo и модуль IDE одним запуском:
./build-workspace.sh --projects iyacc,limbo-bootstrap,ide
# Прямая сборка компиляторов и линкера без CMake:
./build-workspace.sh --backend mk --projects compilers,linker,iyacc
./build-workspace.sh --backend make --projects compilers,linker
```

[Подробная инструкция и границы поддержки](BUILD-NATIVE.ru.md).

Доступные движки: `python3 build-menu.py --list-backends`.
Выбор: `--backend cmake|meson|make|mk|auto`; меню также спрашивает движок.
Meson, Make и mk поддерживают нативную выборку компиляторов/линкера/инструментов;
полная сборка и общие тесты выполняются через CMake 3.10.

### Выбор mk в меню

`mk`, `make` и `meson` пока собирают только `compilers`, `as1`, `as2`,
`as3`, `limbo`, `linker`, `cocor`, `mk`, `iyacc`, `limbo-bootstrap`. Полный набор `all`,
тесты workspace и пример IDE требуют `cmake` или `auto` (CMake 3.10+).
Меню проверяет совместимость до запуска: при `all` + `mk` предлагает
изменить двигатель или заново выбрать компоненты, ничего не исключая автоматически.
Для нативных двигателей тесты доступны для выборки mk/iyacc/limbo-bootstrap; пример IDE недоступен.

```sh
python3 build-menu.py --projects compilers,linker,mk,iyacc --backend mk
```

## Сборка mk, iyacc и Limbo без emu и CMake

```sh
python3 build-menu.py --projects mk,iyacc,limbo-bootstrap --backend mk --tests
```

В интерактивном меню это компоненты `14,15,16`, двигатель `mk`.
Получатся `build/workspace/mk/mk`, `build/workspace/iyacc/iyacc` и
`build/workspace/limbo-bootstrap/limbo-bootstrap`. Это временный нативный
Limbo-компилятор из выбранных исходников вашего Inferno, не интерпретатор.
Он заново собирается из C с парсером, сгенерированным локальным iyacc.
Проверки исполняют mk, генерируют Limbo-парсер и проверяют выпуск Dis, `.sbl`
и C-заготовок. Можно выбрать также `make`, `meson` или `auto`.

Для компиляции IDE, тестовых модулей и консольных примеров emu больше не нужен:

```sh
./build-workspace.sh --projects ide --tests
```

Без `EMU_EXECUTABLE` выполняются проверки продуктов компиляции и Python-хоста,
но не проверки исполнения Dis. Меню не предлагает недоступный запуск примера.
Если явно передать `--demo` без рантайма, сборка всё равно выполняется, после
чего команда сообщает `NOT RUN` и возвращает код 3. Это не успешный запуск IDE.
При общей сборке остальные компоненты и их тесты продолжают выполняться.
Непрошедшая проверка по-прежнему останавливает сборку.

[Какие службы нужны встроенному Dis](disVM/HOST-REQUIREMENTS.ru.md).

## Общий хост FlashVM/Dis

```sh
./build-workspace.sh --projects disvm-host --tests
# Экспериментальная изоляция REG, не готовая параллельная VM:
DVM_NATIVE_REGISTERS=ON ./build-workspace.sh --projects disvm --tests
```

В меню добавлен компонент `17` (`disvm-host`), входящий в `all`.
[Четыре платформы, источники модулей и границы эксперимента](disVM/host/README.ru.md).
[Блокирующие каналы и снимки ожидающих сторон](flashVM/doc/CHANNEL_RENDEZVOUS.ru.md).

## Автоматический выбор Coco/R

Для обычного запуска меню не требуется отдельно устанавливать Coco/R или
настраивать SDK: генератор собирается из tools/coco-r-C, фреймы берутся из
того же проверенного снимка. Старые COCOR_ROOT/COCOR_EXECUTABLE в окружении
в режиме auto проверяются; если они неполные, указывают на удалённую папку
или не содержат все четыре C-фрейма, сборка печатает предупреждение и сама
использует встроенные исходники. Ничего вручную исправлять не нужно.

COCOR_SOURCE=bundled явно выбирает встроенный инструмент. Только для
намеренно выбранного внешнего SDK доступен COCOR_SOURCE=external: тогда
ошибка путей останавливает сборку. Это дополнительная настройка, а не шаг
обычной установки. Полный корректный внешний SDK в auto также поддерживается.
Путь tools/coco-r-C автоматически распознаётся как обёртка над upstream/.

При --tests проверки выбора SDK и написания Coco/R в документации запускаются
автоматически вместе с тестами генератора. Технические имена `cocor`, пути,
COCOR_ROOT и команды в примерах сохраняются для совместимости.

## Module versions and workspace owners / Модули и рабочее пространство

The Dis host package now contains a [versioned native module store](disVM/MODULE-STORE.md),
its public SDK and a fully commented C example. The IDE package contains
[ideworkspace](ide/WORKSPACE.md), debug symbols and a callable Limbo tutorial.
Both are built/installed by the existing disvm-host,ide selection. Their tests
exercise actual channels/native threads; full embedded Dis/IDE startup remains unfinished.

```sh
./build-workspace.sh --projects disvm-host,ide --tests
```

Документы, история и билеты компиляции принадлежат Limbo-сервисам. Хранилище
байтов модулей удерживает прежние версии для читателей и не заменяет парсер,
планировщик или GC. Устаревшие результаты сборки отвергаются по ревизии/билету.


## Runnable desktop and Dis checks / Запуск окна и проверок Dis

```sh
bash build-workspace.sh --projects vm,disvm --vm-profiles player --jobs 4
./out/run-checks.sh
./out/run-checks.sh --window
```

SDL2 SDK при необходимости собирается автоматически, без ручной установки.
`flashvm` в SDL2-пакете запускает настоящее окно, а `flashvm-console` — отдельную
консольную проверку. См. [запуск плеера](flashVM/frontend/RUNNING.md) и
[проверочные программы Dis](disVM/RUNNABLE-CHECKS.md). Проверки Dis/JIT исполнимы
нативно; полноценный хост исполнения произвольных Dis-модулей и IDE ещё не завершён.


## Сборка и запуск из меню без ключей

Запустите `python3 build_menu.py`, введите `all`, остальные вопросы можно
подтверждать Enter. По умолчанию выбираются auto, настоящий оконный FlashVM
с AGG/JIT, вводом и звуком, установка бинарных файлов и примеров в `out/`.
SDL2 SDK при необходимости собирается автоматически.

В меню FlashVM есть четыре варианта: оконный JIT, оконный интерпретатор,
обе оконные сборки и консольная сборка. Интерпретатор устанавливается в
`out/flashvm/agg-sdl2`, JIT — в `out/flashvm/agg-sdl2-jit`.
Дополнительные настройки позволяют изменить число заданий, конфигурацию,
каталоги сборки/установки, профили и установку результатов без ключей командной строки.
После успешной сборки меню показывает пути и предлагает запустить проверки,
оконный пример или оба последовательно. Enter завершает работу без запуска.
Точка входа IDE на нашем DisVM доступна: [запуск и границы готовности](ide/dis/app/README.ru.md). GUI-компиляция/отладчик ещё в разработке.


## Native Dis executable and IDE services / Исполняемый Dis и сервисы IDE

`python3 build_menu.py` → `all` → Enter теперь также собирает `disvm-runtime`
для Linux AMD64. В конце меню: **4** — программа Dis с каналами, **5** —
интерактивный редактор, **6** — демонстрация сервисов IDE. Бинарный файл, согласованный компилятор, `.dis`, `.sbl`
и примеры устанавливаются в `out/disvm-runtime/`.

Это реальное исполнение Limbo/Dis без внешнего `emu`. Используется отдельный
экспериментальный NC64-формат; старые `.dis` не смешиваются с нативными модулями.
Графический редактор и полный набор Sys ещё не подключены. Подробности и
точные границы — в [руководстве рантайма](disVM/runtime/README.md).


## Интерактивный редактор на Dis / Interactive Dis editor

В `build_menu.py` выберите `all`, затем в меню запуска **5**. Доступны `help`,
`show`, `set`, `append`, `insert`, `delete`, `find`, `replaceall`, `undo`, `redo`,
`load`, `save`, `quit`. `complete as ...` / `complete limbo ...` показывают
декларации, `hint LANGUAGE NAME ARGUMENT` — справку по параметрам. Всё выполняется
на собственном Dis с настоящими каналами и реальной файловой записью.
Пункт **6** сохраняет прежнюю автоматическую демонстрацию сервисов.
[Подробное руководство и ограничения](ide/dis/examples/editor-console/README.md).


## Retained modules / Перенесённые модули

В меню после сборки появился пункт **7 — библиотеки Inferno**. Он запускает
настоящую программу Dis с Srv, Math, Loader, Bufio, String, Arg и Styx,
проверяет числовые переходы и обработку повреждённых пакетов. Пример и исходники
устанавливаются рядом с бинарным файлом; run-checks.sh также запускает эту проверку.
[Пример](disVM/examples/standard-modules/README.md) ·
[Готовность каждого модуля](disVM/libraries/README.md).

Post-build item **7** runs the actual standard-module tutorial. Full /prog,
namespace and live Styx-server services remain incomplete; compiled library
availability is distinguished from executed function coverage in the status guide.

## Graphical Dis/FlashVM editor / Графический редактор

Build component `disvm-editor`, then use launch menu **8**. The controller is
Limbo bytecode on our Dis VM; graphics use FlashVM TextField and an optional SDL2
window. Bundled OFL Noto Sans Mono starts at size 11; no font-location prompt. [English](ide/dis/examples/graphical-editor/README.md) ·
[Русский](ide/dis/examples/graphical-editor/README.ru.md). This first graphical
integration does not yet execute AS workers or connect the debugger.

[IDE application entry on our own DisVM](ide/dis/app/README.md) · [Запуск IDE и границы готовности](ide/dis/app/README.ru.md).
