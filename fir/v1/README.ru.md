# Инструменты FIR v1

[English](README.md). Реализация **0.66.0**, текстовый формат **FIR 1.0**.
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

[Compound updates / Составные обновления](doc/COMPOUND_UPDATES.md) · [Русский](doc/COMPOUND_UPDATES.ru.md).

[Unicode string primitives / Строковые операции](doc/UNICODE_STRINGS.md) · [Русский](doc/UNICODE_STRINGS.ru.md).

[Int arithmetic and Unicode bounds / Int-арифметика](doc/INT_ARITHMETIC.md) · [Русский](doc/INT_ARITHMETIC.ru.md).

Compiler 0.28 / Компилятор 0.28: [text expressions and int bits](doc/TEXT_BITS.md), [русское руководство](doc/TEXT_BITS.ru.md).

Compiler 0.29 / Компилятор 0.29: [checked signed32 shifts](doc/INT_SHIFTS.md), [русское руководство](doc/INT_SHIFTS.ru.md).

Compiler 0.30 / Компилятор 0.30: [ordered strings and checked int casts](doc/TEXT_VALUES.md), [русское руководство](doc/TEXT_VALUES.ru.md).

Compiler 0.31 / Компилятор 0.31: [hybrid table constructors](doc/TABLE_LITERALS.md), [русское руководство](doc/TABLE_LITERALS.ru.md).

Compiler 0.32 / Компилятор 0.32: [relative indexes and copy slices](doc/RELATIVE_SEQUENCES.md), [русское руководство](doc/RELATIVE_SEQUENCES.ru.md).


[Удержание таблицы на всё присваивание](doc/TABLE_LEASES.ru.md): захват места, RHS и store/splice; комментированный [leases.b](examples/tables/leases.b).


[While и for-in по таблицам](doc/LOOPS.ru.md) — исполняемые циклы, изменяемые big-локальные и подробный пример.


[If/else, break, continue и ранний return в циклах](doc/LOOP_CONTROL.ru.md).


[Трёхчастный for и постфиксные обновления big](doc/COUNTED_FOR.ru.md).


[Типизированные циклы и обход строк/снимков](doc/TYPED_LOOPS.ru.md).


[Ссылочные переменные в циклах](doc/RETAINED_LOOPS.ru.md): строки, таблицы, снимки и ключи.


[Операторы и области функции](doc/FUNCTION_FLOW.ru.md): обычные условия, блоки, ранние возвраты и свёртка ячеек.


[Гарантированная инициализация через ветви](doc/JOIN_INITIALIZATION.ru.md): типизированные объявления, независимая проверка FIR и nil-хранение.

[Первое присваивание в цикле — 0.43](doc/LOOP_INITIALIZATION.ru.md): типизированные внешние переменные, безопасное чтение в теле и границы фактов на выходе. [Комментированный пример](examples/tables/loop-initialization.b).

[Шаг for в порядке исполнения — 0.44](doc/DEFERRED_STEP.ru.md): доказательство инициализации по всем входящим путям, включая continue. [Пример](examples/tables/deferred-step.b).

[Устранение скалярных загрузок — 0.45](doc/SCALAR_FORWARDING.ru.md): приватные ячейки в одном блоке, сохранение записей, границ CFG и барьеров эффектов.

[Перезаписанные скаляры — 0.46](doc/SCALAR_STORES.ru.md): удаление ненаблюдаемых приватных записей с сохранением последних значений и границ исключений/владения.

[Обход значений с одним именем — 0.47](doc/VALUE_ITERATION.ru.md): таблицы, руны строк и ключи снимков; прежние захват, области имён и управление.

[AS globals/property stores through the common pipeline](doc/AS_PROPERTIES.md) · [Русский](doc/AS_PROPERTIES.ru.md).

[Executable type aliases / Исполняемые псевдонимы](doc/SOURCE_ALIASES.ru.md) · [Tutorial / Пример](examples/type-aliases/README.ru.md)

[GC owner lifetime / Время жизни владельцев](examples/type-aliases/README.ru.md) — runtime 0.32.1, source aliases and real Dis GC backedges.

[Signed32 bit operations / Побитовые операции int](doc/INT_BITS.ru.md) — FIR 0.52, SWL 0.3.12.

[Typed predicates EN](doc/TYPED_PREDICATES.md) / [RU](doc/TYPED_PREDICATES.ru.md): direct int comparisons, strict bool.not/bool.eq and line-commented Dis/SWF tutorials.

[Real conditions EN](doc/REAL_CONDITIONS.md) / [RU](doc/REAL_CONDITIONS.ru.md): ordered binary64 terminal branches, NaN-safe SWF lowering and fully line-commented examples.

[Lazy numeric conditions / Ленивые числовые условия — FIR 0.55](doc/NUMERIC_LOGIC.md) · [Русский](doc/NUMERIC_LOGIC.ru.md).

[Grouped numeric conditions / Группировка числовых условий — FIR 0.56](doc/GROUPED_CONDITIONS.md) · [Русский](doc/GROUPED_CONDITIONS.ru.md).

[Numeric cells and structured source flow / Числовые ячейки и структурный CFG — FIR 0.57](doc/NUMERIC_FLOW.md) · [Русский](doc/NUMERIC_FLOW.ru.md).

[Numeric counted loops / Числовой for — FIR 0.58](doc/NUMERIC_FOR.md) · [Русский](doc/NUMERIC_FOR.ru.md).

[Total integer identities — 0.59](doc/INTEGER_IDENTITIES.md) · [Русский](doc/INTEGER_IDENTITIES.ru.md) · [C99/Dis/SWF tutorial](examples/integer-identities/README.md).

[Знаковые SWF div/mod и удаление неиспользуемой безопасной арифметики](doc/SWF_INT_DIVMOD.ru.md).

[Paired integer rules / парные целочисленные правила](doc/INTEGER_PAIRS.md), [RU](doc/INTEGER_PAIRS.ru.md).

[Известные биты int32 и динамическая арифметика SWF](doc/INTEGER_FACTS.ru.md): общий C99-анализ, сохранение ошибок и пример.

[Биты после сдвигов и big DCE / Shift facts and big DCE](doc/SHIFT_FACTS.ru.md).

[Known bit and arithmetic results / Известные битовые и арифметические результаты](doc/KNOWN_RESULTS.ru.md).
