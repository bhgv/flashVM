# Рабочий числовой source-профиль 0.9.0

Здесь сохранены базовый профиль и этапы его расширения. Для реализации 0.52 см. [актуальное покрытие](COVERAGE.ru.md) и [побитовые операции](INT_BITS.ru.md).

Это реально понижаемый в FIR профиль, отдельно от широких синтаксических фронтендов.
[Грамматика](../compiler/src1.atg), [C99 lowering](../compiler/lower.c).

| Syntax | AS1/AS2/AS3 | Limbo value() |
| --- | --- | --- |
| `+`, `-`, `*`, groups, unary `+/-` | f64 | i32 wrap |
| `.5`, `2.`, `1e2`, `0x2a` | yes | rejected |
| `2r101010`, `16r2a` | rejected | radix 2..36, checked digits/range |
| `var a=1,b=a+1;` | yes | rejected |
| `const x:Number=1;` | AS3 only, immutable | rejected |
| `x: int=1;`, `x:=1;` | rejected | yes |
| `x+=1; x-=1; x*=2;` | yes | yes |
| `# comment` | rejected | yes |
| `;` | empty statement | empty statement |

Приоритет: unary, затем *, затем +/-. Бинарные операции левоассоциативны.
Знак перед литералом допускает i32 minimum; перед группой/переменной минус
понижается как умножение на -1, сохраняя знак нуля AS. `++`/`--` не трактуются
как два знака и отвергаются. Базовая таблица описывает i32; отдельные big/table и real-профили перечислены в актуальном покрытии.
Значения вне диапазона, неверная цифра radix, присваивание const и объявление
const в AS1/AS2 — ошибки с сохранением существующего выходного файла.
Объявления обрабатываются слева направо, без hoisting и без неявных globals.

[AS2 пример](../examples/as2/syntax.as), [AS3 const](../examples/as3/syntax.as),
[Limbo пример](../examples/limbo/syntax.b). Соответствующие syntax.txt содержат
ожидаемые trace. Используйте fir1_source, fir1_opt, fir1 object и swl link,
как в [полном цикле](PIPELINE.ru.md). Потоки и управление через C API сохранены.
У AS2/AS3 доступна только Number-аннотация; нет строк, объектов, if/циклов,
функций AS, каналов Limbo и полного модульного ABI. // и /* */ в Limbo пока
сохранены как совместимость со старыми учебными примерами; родной комментарий #.
Пределы: 64 KiB, 256 имён, 1000 SSA-значений, 64 уровня групп/unary, 255 байтов токена.

В AS дополнительно поддержаны / и /= с приоритетом умножения и левой
ассоциативностью. С версии 0.27 Limbo int поддерживает /, %, /= и %=; живые целочисленные деление и остаток пока поддержаны только в Dis. Проверены деление на -0,
результат NaN и сохранение операций после оптимизации. Вывод source-компилятора
содержит согласованные поля source_language/source_semantics; они сохраняются
при round trip и оптимизации. Деление требует SWL >=0.3.2.

## Терминальные решения Limbo 0.7.0

После пролога принимается return либо дерево `if (a relation b) { result }
else { result }`. Каждый result снова return или такое дерево. Оба арифметических
операнда вычисляются один раз; relations — `<`, `==`, `>`, `!=`, `<=`, `>=`.
Компилятор 0.22 допускает объявления/присваивания внутри int- и big/table-ветвей;
[правила областей](TERMINAL_ARMS.ru.md). Продолжение после решения не поддержано;
компилятор 0.24 добавляет [ленивые &&/||](SHORT_CIRCUIT.ru.md). Big/table принимает
[сравнения-значения и !](COMPARISON_VALUES.ru.md): phi/слияние окружений не изобретаются. Ограничения: общая глубина
64, до 127 терминальных решений и 1000 SSA-значений. Терминальные и логические
блоки делят бюджет 256 блоков вместе со входным.
Coco/R-правила LimboResult/LimboCondition/Relation в [src1.atg](../compiler/src1.atg)
вызывают типизированное lowering в [lower.c](../compiler/lower.c).
[Пример и два backend](../examples/dis/README.ru.md).

## Однородный Limbo real, 0.9.0

Интерфейс `value: fn(): real;` и определение `value(): real` должны совпадать.
Тип выбирается правилами Coco/R ResultType/BindingType. Все значения функции
real: явные `x: real = 2.5`, inferred `x := 2.5`, присваивания и лексические
блоки. Поддержаны + - * /, унарные знаки и return; # сохраняется как Limbo-comment.
Нужны десятичные литералы с дробной частью или экспонентой. Целые, radix/hex,
int-локальные и смешанные casts отклоняются. Терминальные real-сравнения/ветвления поддерживаются по [текущему контракту](REAL_CONDITIONS.ru.md). Это ограничение
профиля, а не полная система типов Limbo. int-функции сохраняют прежний профиль.
`real` остаётся допустимым именем переменной AS1/2/3, но является типовым словом
Limbo. Новый `limbo_real` сохраняется при print/opt и достигает SWO/SWF либо Dis.
[Полный пример](../examples/dis/README.ru.md#вещественный-limbo).

Ошибки диапазона при чтении real-литерала также отклоняются, включая underflow,
если host strtod выставляет errno. Это ограничение текущего числового профиля.

[Checked snapshot keys and exact extension status](EXTENSIONS.ru.md).

[Отражение и стандартный синтаксис cast big/string в Limbo](TYPE_REFLECTION.ru.md).

[Compiler 0.25: captured compound updates / Составные обновления](COMPOUND_UPDATES.ru.md).

[Compiler 0.26: Unicode strings / Строковые операции](UNICODE_STRINGS.ru.md).

Compiler 0.27 / Компилятор 0.27: [int + - * / %, += -= *= /= %=](INT_ARITHMETIC.md),
[русское руководство](INT_ARITHMETIC.ru.md). [Current homogeneous int bits/shifts](INT_BITS.md) / [актуальные биты/сдвиги](INT_BITS.ru.md).

[Lazy numeric conditions / Ленивые числовые условия — FIR 0.55](NUMERIC_LOGIC.md) · [Русский](NUMERIC_LOGIC.ru.md).

[Grouped numeric conditions / Группировка числовых условий — FIR 0.56](GROUPED_CONDITIONS.md) · [Русский](GROUPED_CONDITIONS.ru.md).
