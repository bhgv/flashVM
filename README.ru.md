# Рабочее пространство FlashVM

Меню сборки: `python3 build-menu.py`.

[First steps / Первые шаги](README-FIRSTSTEPS.md) · [English](README-FIRSTSTEPS.en.md)

Версия разработки: **0.1629**. [English](README.md).

Текущая серия: [v0.1629](docs/releases/v0.1629.md): нативная арифметика big, точные пипхол-оптимизации и обновлённые Dis JIT x64/ARM64.

## Просмотр документации

Из корня репозитория:

```sh
python -m pip install -r flashVM/doku_viewer/requirements.txt
python view_docs.py
```

Корневой запуск сейчас открывает [документацию FlashVM](flashVM/README.ru.md).
Он передаёт управление `flashVM/view_docs.py`, сохраняя параметры `--port`
и `--no-browser`. Совместимость с Python 2.7.9 / Python 3 сохраняется;
данное изменение структуры проверено на Python 3, без заявления о новой
проверке исполнения на Python 2.7.9.

## Структура

| Путь | Назначение |
| --- | --- |
| [LICENSE](LICENSE) | Общая лицензия MIT для собственного кода, автор bhgv |
| [ide/](ide/README.ru.md) | Архитектура IDE на Limbo/Dis, UI только через каналы и host-инструменты сборки |
| [compiler/](compiler/README.ru.md) | Независимые синтаксические проекты AS1, AS2, AS3 и Limbo на C99 |
| [linker/](linker/README.ru.md) | Независимый C99-линкер SWO/SWA и ресурсов в SWF8 |
| [flashVM/](flashVM/README.md) | Исходники VM, встроенные зависимости, примеры, документация, инструменты и настройки сборки |
| [flashVM/README.md](flashVM/README.md) | Английское руководство по VM и текущие возможности |
| [flashVM/README.ru.md](flashVM/README.ru.md) | Русское руководство по VM |
| [flashVM/VERSION.txt](flashVM/VERSION.txt) | Версия разработки FlashVM |
| [view_docs.py](view_docs.py) | Вход в документацию рабочего пространства, пока передающий запуск FlashVM |

Синтаксические фронтенды находятся в [compiler/](compiler/README.ru.md).
Отдельная [система FIR](fir/v1/README.ru.md) содержит числовые адаптеры исходников,
верификатор, оптимизатор и генератор объектов SWO. [SWL](linker/README.ru.md)
собирает объекты, библиотеки целыми членами, фигуры, JPEG и PCM WAV в SWF8.
Это работающий ограниченный тракт, а не полная поддержка языков AS/Limbo.
Сторонние зависимости сохраняют свои лицензии внутри компонентов.

## Сборка FlashVM

```sh
cmake -S flashVM -B flashVM/build
cmake --build flashVM/build
ctest --test-dir flashVM/build --output-on-failure
```

Также можно выполнить `cd flashVM` и пользоваться прежними командами проекта.
DEFS, публичные заголовки, структура исходников и внутренние относительные
пути остаются локальными для компонента. См. [ключи сборки](flashVM/DEFS.txt),
[роудмап](flashVM/doc/ROADMAP.md) и [приёмку](flashVM/doc/RELEASE_ACCEPTANCE.md).
Перенос файлов не означает завершения стадий совместимости или платформенных проверок.

## Независимые проекты

FlashVM, синтаксические фронтенды, FIR/компилятор/оптимизатор и SWL — отдельные
работающие проекты. Редактор исходников и изображений Flash пока не реализован.
Каждый проект содержит свои исходники, зависимости, сборку, версию, документацию,
примеры и тесты. Корневая сборка не обязательна; общая автоматизация остаётся
необязательной.

Проект должен собираться после копирования его папки в другое место или отдельный
репозиторий, в том числе после переименования папки. Не подключать исходники
соседних проектов через неявные пути `../`. Необходимые зависимости документировать
и передавать явно как пути к SDK, библиотекам и инструментам либо поставлять внутри
проекта, если лицензия допускает. Компиляторы, линковщик и VM взаимодействуют через
документированные форматы и публичные API; редактор использует явные интерфейсы.

Для отделения FlashVM скопируйте `flashVM/`, а общую корневую `LICENSE` положите
в корень нового дистрибутива рядом с этой папкой. Сохраните лицензии встроенных
зависимостей. Запускайте CMake и `view_docs.py` внутри скопированного проекта.
Просмотрщик компонента обслуживает только его папку; корневой запуск явно включает
навигацию по рабочему пространству. Утилита архивирования истории Git требует
исходный Git-репозиторий, но для сборки и работы отдельной VM не нужна. История Git
и вспомогательные файлы общего корня не являются зависимостями сборки.

## Проекты компиляторов и линковщика

В [compiler/](compiler/README.ru.md) находятся независимые C99-фронтенды
[AS1](compiler/as1/frontend/README.ru.md), [AS2](compiler/as2/frontend/README.ru.md),
[AS3](compiler/as3/frontend/README.ru.md) и [Limbo](compiler/limbo/frontend/README.ru.md).
Каждый использует читаемую исходную `.atg`, внешний bhgv/coco-r-C, C API,
консольный драйвер и синтаксические тесты. Версии независимы: см. VERSION/README
соответствующего проекта.

Отдельные числовые [адаптеры исходников](fir/v1/doc/SOURCE_SYNTAX.ru.md) выдают FIR.
Проверка и оптимизация FIR предшествуют генерации SWO; компилятор FIR не создаёт
контейнер SWF. [SWL](linker/README.ru.md) объединяет SWO/SWA и ресурсы; расширения
FlashVM требуют явного разрешения. Широкое распознавание синтаксиса не означает
полное семантическое преобразование: AS3 ABC и межмодульные релокации функций FIR
ещё не реализованы. [Справка тракта](fir/v1/doc/PIPELINE.ru.md),
[примеры SWL](linker/examples/README.ru.md),
[типизированный C API просмотра](linker/INSPECTION.ru.md).

## Десктопный проигрыватель SWF

Необязательный [frontend FlashVM](flashVM/frontend/README.ru.md) содержит запуск
для Linux, Windows и macOS, справку `-h`/`--help`, interpreter/JIT, качество,
абсолютный FPS и скорость в процентах. Нужен SDK разработки SDL2:

```sh
cmake -S flashVM -B flashVM/build-desktop -DFVM_BUILD_DESKTOP_FRONTEND=ON
cmake --build flashVM/build-desktop --target flashvm_player --parallel
./flashVM/build-desktop/frontend/flashvm_player --help
```

## FIR v1

[Формат, C99/Coco-R инструменты и примеры](fir/v1/README.ru.md).

[Стиль ATG и проверка оформления](tools/README.ru.md).

[Нативная и cross-сборка всех компонентов](BUILDING.ru.md),
[каталог примеров компиляторов](compiler/examples.ru.md).

[Парсеры AS1/AS2/AS3/Limbo/FIR: серия 0.6.0](compiler/PARSER_COVERAGE.ru.md).

## Продолжение компиляторов и IDE

[FIR 0.5.3: лексические блоки Limbo](fir/v1/doc/LEXICAL_SCOPES.ru.md) ·
[IDE 0.3.1: навигация/поиск через каналы](ide/dis/examples/navigation/README.ru.md).
Reply ABI изменён в 0.3.0: пересоберите ядро и клиентов вместе. Реализованы
ограниченные рабочие компоненты; полные компиляторы, встраивание Dis, JIT
и execution debugger ещё не прошли итоговую приёмку.

[FIR 0.5.4 source diagnostics](fir/v1/doc/SOURCE_DIAGNOSTICS.md) ·
[IDE 0.4.0 Undo/Redo](ide/dis/HISTORY.md) ·
[Limbo history tutorial](ide/dis/examples/history/README.md).

[Compiler/IDE 0.1526 acceptance and remaining VM regressions](ide/dis/SERIES_0_4_0.md).

[DisVM interpreter/JIT core](disVM/README.md) ·
[Owned tool sources and revisions](tools/UPSTREAMS.md) ·
[.m: external AS modules for Dis and AS callers of Dis](compiler/limbo/examples/external-modules/README.md).

[Compiler signatures / Сигнатуры компилятора](compiler/limbo/frontend/SIGNATURES.ru.md) · [IDE ReplaceAll](ide/dis/REPLACE_ALL.ru.md)

[CMake 3.10 / Make / Inferno mk](BUILD-NATIVE.ru.md)

[Отдельное сравнение GC](docs/GC-COMPARISON.ru.md).

[Channel editor / Канальный редактор](ide/EDITOR-CHANNELS.ru.md) · [AS view/worker](ide/as/README.md) · [Dis services](ide/dis/bridge/README.md).
