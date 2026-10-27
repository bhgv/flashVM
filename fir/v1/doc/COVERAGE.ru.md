# Покрытие и ограничения

[English](COVERAGE.md) · [Числовой синтаксис](SOURCE_SYNTAX.md) ·
[Области имён](LEXICAL_SCOPES.ru.md) · [Диагностика](SOURCE_DIAGNOSTICS.ru.md)

| Компонент | Реализовано | Пока отсутствует |
| --- | --- | --- |
| Читатель FIR | Coco/R, UTF-8, комментарии, FIR 1.0 | Будущие версии, бинарный формат |
| Verifier | SSA, доминирование, типы, метки, достижимость, терминаторы | Межпроцедурный анализ |
| Числовые AS1/AS2/AS3 | trace, var с инициализацией, AS3 const, чтение/присваивание, Number-аннотации AS2/3 | Объекты/строки, исходные if/циклы/функции, hoisting, полные языки |
| Числовой Limbo | Учебный value(): int, int/выводимые имена, лексические блоки/затенение, присваивание, терминальные if/else с return | Общий module ABI, функции/import, исходные каналы, alt/spawn, big/real |
| Выражения | Литералы, группы, ограниченные unary +/-, + - *, составные присваивания; AS / и /= | Деление Limbo, общие bool-выражения/SSA joins, другие типы |
| Каналы FIR | Create/send/receive/close через FlashVM extensions | Понижение исходных каналов Limbo, графы модулей |
| Ошибки исходника | Первая ошибка C99 API и необязательный JSON schema 1 с байтовыми диапазонами | Несколько ошибок, полная карта AST, реентерабельность |
| Backend | SWO1 AVM1; отдельный SWL собирает SWF8 | ABC/AVM2, импорты/релокации функций, исключения |

AS использует binary64, включая деление, NaN и signed zero. Limbo — проверяемые
литералы i32 и wrap-арифметику + - *; минус перед группой/переменной поддержан.
AS допускает hex/decimal/exponent, Limbo — основания 2..36. ++/-- отвергаются.
Точный синтаксис/приоритеты описаны отдельно; Coco/R-грамматика авторитетна.

AS-имена плоские, инициализируются слева направо без hoisting. В Limbo блоки
закрывают локальные имена и восстанавливают внешние привязки; присваивания
внешним именам сохраняются. Пределы: вход 64 КиБ, токен 255 байт, всего 256
принятых имён, 1000 SSA-значений и общая вложенность 64 для блоков/групп/unary.
Отклонённая трансляция не перезаписывает назначение; окончательный I/O-copy не
обещает откат при произвольном сбое выхода.

Учебный Limbo выводит value(), не реализуя loader ABI Inferno. AS3 здесь —
арифметическая AVM1-совместимость; полный AS3 требует ABC backend.
Независимые [синтаксические фронтенды](../../../compiler/README.ru.md) распознают
больше конструкций, но это само по себе не означает их понижение/исполнение.

FIR пишет объекты; [линкер](../../../linker/README.ru.md) объединяет
самодостаточные объекты, библиотеки и ресурсы. Оптимизация явная. Диагностика
не является приёмкой встроенного Dis/JIT, полного отладчика, вызовов между
модулями или целевых нативных backend.

[Цель NC64 Dis 0.9.0](DIS_BACKEND.ru.md): скалярный CFG, i32/bool/f64, copy, сравнения/wrap i32 и арифметика binary64, значения и флаги trace/return.
FIR→Dis выполняет наш C99-emitter; полный Limbo ABI, phi, каналы, SBL и Dis-линковка ещё не реализованы.

Числовой Limbo теперь принимает шесть сравнений в терминальных if/else;
все ветви возвращают значение, SSA joins и исходные циклы не реализованы.

Our Coco/R Limbo adapter now additionally admits a homogeneous `value(): real`
profile: decimal real constants, typed/inferred real locals, lexical scopes,
assignments, + - * / and return. No mixed types/casts or real comparisons/CFG
are implied. [Exact grammar coverage](SOURCE_SYNTAX.md#homogeneous-limbo-real-090).

Наш Limbo real — ограниченный однородный профиль с явными real-литералами;
обычные int-функции и их терминальные ветвления сохраняют прежнюю семантику.
[Комментированный пример](../examples/dis/README.ru.md#вещественный-limbo).

[Dynamic any: implemented status and proposed ADT boundary / Состояние any и проект ADT](DYNAMIC_VALUES.md).

[Compiler 0.23: comparison values / Сравнения как значения](COMPARISON_VALUES.ru.md).

Compiler 0.27 / Компилятор 0.27: [int + - * / %, += -= *= /= %=](INT_ARITHMETIC.md),
[русское руководство](INT_ARITHMETIC.ru.md). Int bitwise/shift operations remain unsupported.

Compiler 0.29 / Компилятор 0.29: [checked int shifts](INT_SHIFTS.md),
[проверяемые int-сдвиги](INT_SHIFTS.ru.md), big/table source profile and raw FIR.
Live checked shifts are Dis/reference-only; homogeneous int/AS source profiles are unchanged.

Compiler 0.30 / Компилятор 0.30: [ordered strings and checked int casts](TEXT_VALUES.md),
[порядок строк и проверяемые int-cast](TEXT_VALUES.ru.md), in the typed big/table source profile.
