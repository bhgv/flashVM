# Инструменты FIR v1

[English](README.md). Реализация **0.24.0**, текстовый формат **FIR 1.0**.
Код — C99. Все три парсера генерируются внешним
[bhgv/coco-r-C](https://github.com/bhgv/coco-r-C), тем же генератором, что
используют синтаксические фронтенды AS1/AS2/AS3/Limbo. Сгенерированный код остаётся
в каталоге сборки и не хранится в Git.

## Сборка из корня репозитория

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
COCO_DIR="$TMPDIR/coco-r-C"
git clone https://github.com/bhgv/coco-r-C.git "$COCO_DIR"
make -C "$COCO_DIR/sources" -f unix.mk
cmake -S fir/v1 -B build/fir1 \
  -DCOCOR_ROOT="$COCO_DIR" \
  -DCOCOR_EXECUTABLE="$COCO_DIR/sources/cocor"
cmake --build build/fir1
ctest --test-dir build/fir1 --output-on-failure
```

Linux/Termux: нужны компилятор C99, make, CMake; Python 3 нужен для тестов.
LLVM, SDL и заголовки VM для сборки инструментов не нужны. В Windows укажите
реальный путь к внешнему генератору и используйте C99 toolchain. Windows и
физический Android в этом выпуске не проверены. Отдельно скопированный или
переименованный v1 собирается с теми же параметрами; сохраните корневую MIT LICENSE.

## Руководства

- [Формат и примеры](doc/FORMAT.ru.md): реализованная грамматика и семантика.
- [Покрытие](doc/COVERAGE.ru.md): исходные профили, AVM1 и будущие возможности.
- [Компилятор и API](compiler/README.ru.md): параметры CLI и интерфейсы C99.
- [Peephole](optimiser/README.ru.md): преобразования и ограничения.
- [Примеры](examples/README.ru.md): четыре адаптера, ветвления и каналы.
- [Проверки и хост выполнения](doc/TESTING.ru.md).

## Просмотр

```sh
python -m pip install -r flashVM/doku_viewer/requirements.txt
python fir/v1/view_docs.py --no-browser --port 8766
# Или: python view_docs.py
```

Оба просмотрщика выводят все руководства и примеры FIR. Локальные launchers
есть также в `fir/`, `doc/`, `compiler/`, `optimiser/`, `examples/`. Они используют
общий движок и границу репозитория, поэтому ссылки на API FlashVM работают.
Для отдельной сборки инструментов движок просмотра не требуется; документы
в отделённой копии можно читать как Markdown.

## Изменение backend в 0.4.1

`fir1 swf` и `fir1 actions` удалены. Используйте `fir1 object INPUT OUTPUT.swo`,
затем [SWL](../../linker/README.ru.md): `swl link OUTPUT.swf INPUT.swo`.
Прежние команды в исторических примерах заменяются этим двухэтапным путём.
Для тестов выполнения задайте `FIR_SWL` вместе с `FIR_FLASHVM_RUNNER`.

[C API и встраивание](compiler/API.ru.md).

[Сборка и объектный pipeline](doc/PIPELINE.ru.md). [Пример IDE](../../linker/examples/ide/README.ru.md).

[Рабочий source-синтаксис](doc/SOURCE_SYNTAX.ru.md).

[Стиль ATG и проверка оформления](../../tools/README.ru.md).

Исходники C99 оформлены с отступом 4 пробела, раскрытыми блоками и явными
секциями обработки ошибок. Локальная [.clang-format](.clang-format) сохраняется
при отделении проекта. Генерируемый Coco/R и внешние библиотеки не форматируются.

[Сборка для своей и другой ОС/CPU](../../BUILDING.ru.md).

[Расширенный структурный парсер и C API](frontend/README.ru.md).

[0.5.2: literal module storage](doc/MODULE_STORAGE.ru.md).

[Limbo lexical blocks / Лексические блоки](doc/LEXICAL_SCOPES.md).

[Source diagnostic schema 1 / Диагностика исходников](doc/SOURCE_DIAGNOSTICS.md) ·
[C99 diagnostic example / Пример C99](examples/diagnostics/README.md).

[Own compiler priority / Приоритет Limbo](../../compiler/limbo/OWN_COMPILER.md) ·
[Two-way .m external AS/Dis modules / Внешние модули](../../compiler/limbo/examples/external-modules/README.md) ·
[Owned tools / Инструменты](../../tools/UPSTREAMS.md).

[Source compilation debug info / Debug info собственного компилятора](doc/SOURCE_DEBUG.md).

[Прямая генерация NC64 Dis](doc/DIS_BACKEND.ru.md) · [Limbo/FIR/Dis и библиотечный пример](examples/dis/README.ru.md).

[Типизированные таблицы → FIR](doc/TABLES.ru.md) · [English](doc/TABLES.md): компилятор 0.19, явный профиль с результатом big и эталонное исполнение на C99; [генерация Dis для таблиц](doc/TABLES_DIS.ru.md) реализована; SWF ещё предстоит.

[Сквозная реализация расширений Limbo: таблицы, проекции и any](doc/EXTENSIONS.ru.md).

[typeof и явные приведения Limbo](doc/TYPE_REFLECTION.ru.md).

[Арифметика big](doc/BIG_ARITHMETIC.ru.md).

[Деление, остаток и 64-битные маски](doc/BIG_BITWISE.ru.md).

[Скалярный CFG с big](doc/BIG_CFG.ru.md).

[Exact big source decisions / Точные решения big](doc/BIG_DECISIONS.md) · [Русский](doc/BIG_DECISIONS.ru.md).

[Acyclic table decisions / Ациклические решения таблиц](doc/TABLE_CFG.md) · [Русский](doc/TABLE_CFG.ru.md).

[Typed big shifts / Типизированные сдвиги big](doc/BIG_SHIFTS.md) · [Русский](doc/BIG_SHIFTS.ru.md).

[Table count/in/delete / Подсчёт, наличие и удаление](doc/TABLE_MUTATION.md) · [Русский](doc/TABLE_MUTATION.ru.md) · [Tutorial](examples/tables/mutation.b).

[Compiler 0.22: terminal lexical arms / Лексические ветви](doc/TERMINAL_ARMS.md) · [Русский](doc/TERMINAL_ARMS.ru.md).

[Comparison values / Сравнения как значения](doc/COMPARISON_VALUES.md) · [Русский](doc/COMPARISON_VALUES.ru.md).

[Lazy logic / Короткое замыкание](doc/SHORT_CIRCUIT.md) · [Русский](doc/SHORT_CIRCUIT.ru.md).
