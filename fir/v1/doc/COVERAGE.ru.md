# Актуальное покрытие — FIR 0.69

[English](COVERAGE.md) · [Грамматика](../compiler/src1.atg) · [Публичный API](../include/fir.h) · [Архитектура компиляторов](../../../compiler/README.ru.md)

Здесь описаны исполняемые адаптеры FIR. Более широкие самостоятельные синтаксические фронтенды и исходный bootstrap — отдельные компоненты. Распознанное дерево ещё не означает понижение и исполнение программы.

| Компонент/профиль | Исполняемые возможности | Граница |
| --- | --- | --- |
| Читатель/verifier FIR | Coco/R, FIR 1.0, UTF-8, SSA, доминирование, типы, переходы, терминаторы | Нет бинарного FIR и межпроцедурного анализа |
| Исходные AS1/2/3 | Числовой trace, инициализированные переменные, AS3 const, аннотации и арифметика | Учебная совместимость AVM1, не полный AS/ABC-компилятор |
| Однородный int Limbo | Signed32-арифметика, деление/остаток, биты, проверяемые сдвиги, псевдонимы, типизированные ячейки, if/else/while/for, переходы и группированные условия | Нет неявного смешения bool/int и общего ABI модулей |
| Однородный real | Binary64-арифметика, локальные значения/псевдонимы, ячейки, if/else/while/for, ранний return и группированные условия | Нет неявных смешанных преобразований, ABI вызовов |
| Типизированный big/table | Точные big/int, bool и логика, Unicode-строки, гибридные таблицы, снимки ключей, конструкторы, срезы/мутации и циклы | Ограниченная учебная value(), не общий компилятор составных типов/функций |
| Псевдонимы | Опережающий файловый/модульный префикс, последовательные локальные области и приведения | 128 объявлений на запрос; нет import, qualified/generic и локальных опережающих ссылок |
| Оптимизатор | Проверяемые тождества, DCE доказанно безопасной арифметики, CFG и консервативные эффекты памяти | Не whole-program; ошибки производителей операндов сохраняются |
| Dis | Прямой скалярный и хостовый NC64-бэкенды | Общая линковка Dis, исходные каналы и полная миграция ABI — отдельные этапы |
| SWF | SWO1/SWL/SWA1/ресурсы в SWF8; биты, доказанные int32 div/mod/сдвиги и AS-свойства FIR | Неизвестные делители/счётчики отвергаются; нет ABC/AVM2 |
| C99 API/позиции | Общие CLI/библиотеки, сохранение файлов при отказе, байтовые позиции UTF-8 | Парсер сериализован; полного подключения IDE-дебаггера нет |

## Контракты и примеры

- [Синтаксис](SOURCE_SYNTAX.md), [области значений](LEXICAL_SCOPES.ru.md), [псевдонимы](SOURCE_ALIASES.ru.md), [диагностика](SOURCE_DIAGNOSTICS.ru.md).
- [Биты и сдвиги int](INT_BITS.ru.md), [арифметика](INT_ARITHMETIC.ru.md), [типизированные сдвиги](INT_SHIFTS.ru.md), [преобразование текста/ключей](TEXT_VALUES.ru.md).
- [Конструкторы](TABLE_LITERALS.ru.md), [владение таблицами](TABLES_DIS.ru.md), [Unicode](UNICODE_STRINGS.ru.md), [замена срезов](SLICE_ASSIGNMENT.ru.md).
- [Состояние any](DYNAMIC_VALUES.md), [Dis-бэкенд](DIS_BACKEND.ru.md), [прокомментированные примеры](../examples/README.ru.md).

Пределы: исходник 64 КиБ, токен 255 байт, 256 имён значений, 128 объявлений типов, 1000 SSA-значений, общая глубина 64. Где оговорено, учитываются и вышедшие из области объявления. Отказ трансляции сохраняет прежние FIR/debug-файлы; сбой последующего копирования через host I/O не является транзакционным откатом.

Учебная обёртка вызывает/трассирует value(), а не общий Inferno init. Runtime/IDE пока также использует явно собранный из исходников bootstrap для сервисных модулей. Библиотеки каналов/проекций и канальные операции сырого FIR не означают исходное понижение chan/alt/spawn этим адаптером. ARM64-эмиссия/эмуляция не заменяет испытания на устройстве.

[Typed predicates EN](TYPED_PREDICATES.md) / [RU](TYPED_PREDICATES.ru.md): direct int comparisons, strict bool.not/bool.eq, optimizer rewrites, scalar SWF and table-backed Dis examples.

[Real conditions EN](REAL_CONDITIONS.md) / [RU](REAL_CONDITIONS.ru.md): ordered binary64 predicates, finite subnormal literals and NaN-safe Dis/SWF lowering.

[Lazy numeric conditions / Ленивые числовые условия — FIR 0.55](NUMERIC_LOGIC.md) · [Русский](NUMERIC_LOGIC.ru.md).

[Grouped numeric conditions / Группировка числовых условий — FIR 0.57](GROUPED_CONDITIONS.md) · [Русский](GROUPED_CONDITIONS.ru.md).

[Structured numeric flow EN](NUMERIC_FLOW.md) / [RU](NUMERIC_FLOW.ru.md).

[Numeric for EN](NUMERIC_FOR.md) / [RU](NUMERIC_FOR.ru.md).

[Деление/остаток SWF и DCE безопасной арифметики](SWF_INT_DIVMOD.ru.md): доказательство, C99/IDE, пример и правила отказа.

[Парные целочисленные сокращения/поглощение](INTEGER_PAIRS.ru.md): точные правила с сохранением ошибок производителей.

[Known int32 bits / Известные биты int32](INTEGER_FACTS.ru.md): bounded dynamic counts/nonzero values; no memory inference.

[Shift/big facts / Сдвиги и big](SHIFT_FACTS.ru.md).

[Known bit constants / Точные битовые константы](KNOWN_RESULTS.ru.md).

[FIR0.69 carry/borrow facts and exhaustive arithmetic evidence / Переносы,
заёмы и исчерпывающие проверки](KNOWN_RESULTS.md#carry-and-borrow-facts).
