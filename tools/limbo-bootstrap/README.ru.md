# Временный native bootstrap Limbo из исходников

[English](README.md) · [Приоритет собственного компилятора](../../compiler/limbo/OWN_COMPILER.ru.md) ·
[Обновление](../UPSTREAMS.ru.md)

Источник: https://github.com/bhgv/Inferno-OS-bhgv, ветка experiment. Commit и
SHA-256 — в UPSTREAM.json. Перенесены C-версия limbo/ и Limbo-версия
appl/cmd/limbo/, в том числе sbl.b/stubs.b как база для нашей разработки.
Это не выдаётся за наш Coco/R-компилятор: приоритет остаётся у compiler/limbo + FIR.

Bootstrap нужен потому, что числовой Limbo→FIR пока не собирает весь код IDE и
Coco/R-генератора. Теперь этот пробел закрывается собственной source-сборкой,
без готового чужого SDK. Native tool — временный инструмент сборки, не runtime
IDE. Исторический generated yacc-парсер сохранён как bootstrap-исходник;
устанавливать yacc не нужно. Наши новые парсеры остаются Coco/R.

```sh
./build-workspace.sh --projects limbo-bootstrap --tests
./build-workspace.sh --projects ide
```

Выборка включает lib9/UTF/форматирование, buffered I/O, преобразование чисел,
нужные .m и два MD5-файла для fingerprint типов Limbo. Сохранён исходный notice
RSA Data Security, Inc. MD5 Message-Digest Algorithm. Внешнего crypto SDK,
Keyring-сервиса, реализации GUI, полной ОС и готовых бинарников нет.
POSIX/fenv-адаптеры отдельны от upstream. ARM-only inline возвратного адреса
заменён на границе include compiler intrinsic; ARM-ассемблер не собирается.
Аппаратные FP traps bootstrap маскирует. Это не готовый host-порт всей VM/FPU.

## Debug и native-заготовки

Тесты проверяют unsigned Dis XMAGIC, -g/.sbl 2.1 и старый генератор C через -a.
В legacy-компиляторе -a даёт объявления, -t MODULE — runtime tables; исходники
сохранены для изучения и переноса возможностей в наш компилятор.

```sh
mkdir -p build/output
build/workspace/limbo-bootstrap/limbo-bootstrap -g -I ide/dis -o build/output/idecore.dis ide/dis/idecore.b
build/workspace/limbo-bootstrap/limbo-bootstrap -a ide/dis/idecore.m > build/output/legacy-ide-declarations.h
```

Сборка bytecode и проверка source map не означают приёмку исполнения VM.
Legacy compiler сохраняет исходные 32-битные соглашения данных/фреймов Dis;
новые 64-битные leaf JIT сами по себе не завершают native frame/layout port.
Компилятор и эмулятор из разных forks нельзя автоматически считать совместимыми.
Для IDE --tests/--demo по-прежнему нужен явный совместимый EMU_EXECUTABLE;
при другом runtime задавайте и его matching compiler/header overrides.

## Прямая сборка через mk, Make или Meson

```sh
./build-workspace.sh --backend mk --projects mk,iyacc,limbo-bootstrap --tests
```

Команда собирает настоящие mk и iyacc, заново генерирует парсер из `limbo.y`,
компилирует нативный Limbo через CC/ar и проверяет продукты всех трёх инструментов.
Ни CMake, ни emu не требуются. Та же команда работает с `--backend make` и
`--backend meson` (для последнего нужны Meson/Ninja).
`bootstrap.sh` использует явные списки `*.sources`, сохраняет порядок библиотек
из standalone CMake и пишет только в каталог сборки. Поддерживаются `CC`, `AR`,
`CPPFLAGS`, `CFLAGS`, `LDFLAGS`, как у остальных нативных инструментов.
Для прямого вызова нужны уже собранный iyacc и его каталог со скелетом парсера:

```sh
IYACC_EXECUTABLE="$PWD/build/workspace/iyacc/iyacc" \
IYACC_ROOT="$PWD/build/workspace/iyacc" \
  bash tools/limbo-bootstrap/bootstrap.sh build/workspace/limbo-bootstrap
```

Модули времени исполнения и полная установка Inferno этому пути не нужны.


## Имена констант big на LP64

В C-версии найдена коллизия: два 64-битных `long` в 32-байтном имени могли
обрезаться, и -2/-5 превращались в одну константу. Сборка теперь подготавливает
производный `ecom.c` с двумя явно маскированными 32-битными половинами. Изменение
общее для CMake и прямой сборки; закреплённые исходники не меняются. Python 3
используется только при сборке. [Исправление для Inferno](patches/0001-bound-big-constant-names-on-lp64.patch)
и [проверка настоящего вывода компилятора](tests/big_constants.py).


## Подготовка dtoa и парсера

При сборке Math обнаружен выход за границы double в dtoa на LP64: его long
должен был быть 32-битным. Производный файл теперь использует 32-битные int,
проверяет ширину типа и упаковывает половины через беззнаковый сдвиг. Изменение
используется в обеих сборках; исходный снимок не меняется.
[Патч](patches/0002-use-32-bit-dtoa-limbs.patch). Настоящие компилятор и Dis
проверяют 134 binary64-константы, включая граничные случаи, также под UBSan.
Прямая сборка свежего iyacc-парсера теперь тоже использует общую защиту sentinel.

[Rune provider integration test](tests/rune_providers.c) links the public bounded/allocated wrappers and Fmt builder against the actual selected archive. It checks zero-capacity nonmutation, Unicode scalar truncation, multi-growth cursor retention and caller-owned flush/free. / Проверка публичных Rune-функций связывается с настоящим архивом библиотеки: нулевая ёмкость, обрезка по рунам, рост буфера и сохранение владения после flush.

The current pinned Inferno parser already reserves and checks its sentinel, recovery
pops and empty reductions. `prepare_parser.py` preserves those guards and applies
a bounded 1024-slot host capacity after the upstream header, which otherwise
sets 200. Historical source anchors remain checked; this does not change the
primary Coco/R compiler or the upstream snapshot.

## Общий кодировщик операндов

Снимок включает include/dis-operand.h вместе с limbo/dis.c. Потоковый и
буферный writer используют проверяемый signed30 encoder с беззнаковыми
сдвигами и без частичной записи при отказе по диапазону/ёмкости. Тест
dis-constants проверяет оба production writer и fixed-word output. Байты
классического формата сохраняются; новый Dis64 ABI этим не включается.
