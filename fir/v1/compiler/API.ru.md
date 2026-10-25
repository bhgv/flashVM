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
скалярного i32/bool CFG-профиля. `fir_run(FIR_DIS, ...)` использует ту же реализацию,
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
