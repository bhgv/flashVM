# Встраиваемые C API 0.7.0

[FIR API](../include/fir.h) доступен без argv, stdout/stderr и запуска процессов.
Консольные драйверы только открывают файлы, выбирают операции и печатают ошибки.

| API | CMake target | Operation |
| --- | --- | --- |
| `fir_source(input, output, language, module_name, error, capacity)` | `fir1_source_api` | AS1/AS2/AS3/Limbo numeric subset → FIR |
| `fir_run(task, input, output, options, error, capacity)` | `fir1_ir` | `FIR_CHECK`, `FIR_PRINT`, `FIR_OPT`, `FIR_OBJECT`, `FIR_DIS` |
| `fir_optimise(module, error, capacity)` | `fir1_ir` | In-place verified optimization |
| `fir_object(output, module, error, capacity)` | `fir1_ir` | FIR → SWO1 |

Потоки принадлежат вызывающему коду и не закрываются библиотекой; чтение
начинается с текущей позиции, запись — в текущую позицию. Ошибка возвращает 0,
успех 1. Диагностика — буфер вызывающего кода, допустимо NULL/0. `module_name`
без @ (NULL означает tutorial), максимум 254 ASCII-байта. При связывании разных
исходников задайте разные имена. CLI: `fir1_source as2 --input a.as a.fir --module a`.
`FirRunOptions.optimise` включает оптимизацию перед объектной компиляцией;
по умолчанию она неявно не запускается. `snapshot(user, phase, module)` вызывается
с before/after и позволяет IDE сохранить FIR через `fir_print`. Указатель модуля
действителен только во время callback; возврат 0 отменяет операцию. Callback не
должен рекурсивно запускать парсер. `fir1_opt INPUT OUTPUT` вызывает тот же API.

FIR и source-парсеры имеют разные символы и могут линковаться в одну программу.
Каждый вызов сбрасывает ошибки Coco/R, включая вызов после неверного ввода.
Встроенные фатальные пути сканера перехватываются и возвращаются как ошибка.
Парсеры ещё не reentrant: сериализуйте все обращения к одному парсеру внешним
мутексом. Оптимизация разных caller-owned модулей и независимые SWL-контексты
не используют состояние парсера. Не меняйте один модуль одновременно.

[Test C](../tests/embed.c) связывает оба компилятора, повторяет ошибки и успешные
компиляции четырёх профилей, вызывает snapshots и проверяет SWO1 без subprocess.
Генерируемый код остаётся в build; wrappers reset/guard — обычный C99, грамматики
по-прежнему `.atg`. Полные синтаксические фронтенды — отдельные API/проекты.

## Source diagnostics 0.5.4

`fir_source_ex` adds an optional caller-owned FirSourceDiagnostic result after
the existing arguments. `fir_source` remains a wrapper. First error wins,
success clears the result, positions use original UTF-8 bytes.
[English contract](../doc/SOURCE_DIAGNOSTICS.md) ·
[Полный контракт на русском](../doc/SOURCE_DIAGNOSTICS.ru.md).

## Прямой Dis API 0.6.0

`fir_dis(output, module, error, capacity)` генерирует NC64 Dis для проверенного
скалярного i32/bool/f64 CFG-профиля. `fir_run(FIR_DIS, ...)` использует ту же реализацию,
с необязательной оптимизацией/снимками. Ошибка покрытия не изменяет поток;
NULL-диагностика допустима, ошибка I/O может оставить префикс.
[Точный контракт и пределы](../doc/DIS_BACKEND.ru.md),
[клиент без дочерних процессов](../examples/dis/main.c).

## CFG и решения 0.7.0

`FIR_DIS` поддерживает проверенные i32/bool-блоки, сравнения и рёбра CFG.
Для N живых O_TRACE сначала записаны N значений, затем N флагов исполнения.
Подсчитайте N через callback `after` после оптимизации и публикуйте метаданные
только после успешного результата. `fir_source` принимает терминальные Limbo
if/else; FIRDBG1 добавляет compare/branch с диапазонами оператора и if.
Verifier отвергает перемежающиеся живые блоки и скрытый entry после tombstone.
[Контракт](../doc/DIS_BACKEND.ru.md).

## Binary64 Dis 0.8.0

The same `fir_dis` / `fir_run(FIR_DIS, ...)` API now admits f64 constants, copy,
and add/subtract/multiply/divide. Byte output is identical to the CLI; ownership,
serialization and rejection contracts are unchanged. Numeric AS1/2/3 source
profiles emit explicit f64; dynamic coercion and strings remain rejected.
Real capture cells retain binary64 bits; executed flags remain native words.
The caller supplies type knowledge; no self-describing debug/type map is implied.

Те же C99 API принимают f64-константы, copy и четыре арифметические операции.
Типы захватов известны клиенту из FIR; флаги исполнения остаются целыми.
Правила владения, сериализации вызовов и отказа до записи сохранены.

## Homogeneous Limbo real / Однородный Limbo real, 0.9.0

`fir_source*` still selects FIR_LIMBO. Its Coco/R result-type productions choose
i32 or real; there is no extra CLI/API switch. Source metadata is emitted only
after successful staged parsing, so interface/type mismatch preserves caller
FIR and debug streams. FirSemantics appends FIR_LIMBO_REAL without renumbering
older values. The C99 regression alternates real/failure/int/AS eight times in
one process and verifies metadata, borrowed streams and direct Dis emission.

Профиль задаёт исходная грамматика, вызов остаётся FIR_LIMBO. Несовпадение типов
интерфейса/определения не публикует ни FIR, ни debug. Вызовы по-прежнему должны
сериализоваться хостом; добавление real не делает парсер реентерабельным.

[FIR 0.17: big scalar CFG](../doc/BIG_CFG.md) · [Русский](../doc/BIG_CFG.ru.md).

[FIR 0.18: exact big comparisons/source decisions](../doc/BIG_DECISIONS.md) · [Русский](../doc/BIG_DECISIONS.ru.md).

[FIR 0.19: acyclic owner CFG and fir_acyclic](../doc/TABLE_CFG.md) · [Русский](../doc/TABLE_CFG.ru.md).

[FIR 0.20: typed shifts and explicit int counts](../doc/BIG_SHIFTS.md) · [Русский](../doc/BIG_SHIFTS.ru.md).


Компилятор 0.41 сохраняет сигнатуры API и последовательное обслуживание запросов,
добавляя [операторы и области функции](../doc/FUNCTION_FLOW.ru.md) в профиль Limbo
big/table. Повторные запросы C99 проверяют восстановление после ошибки
инициализации на сходящихся путях.

[Source aliases / Псевдонимы исходника](../doc/SOURCE_ALIASES.ru.md): FIR 0.51, serialized request-local binding.
