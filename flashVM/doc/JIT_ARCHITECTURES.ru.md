# JIT: архитектуры, отладка, потоки и несколько VM — v0.348

Работают два способа выполнения одних и тех же исходных AVM1 actions:
интерпретация и экспериментальный JIT. В JIT генерируются машинные вызовы общих
обработчиков и машинные переходы. Формат байткода разбирается при компиляции;
готовое тело не исполняет цикл диспетчеризации AVM1. Неподдерживаемые инструкции
автоматически возвращают выполнение к интерпретатору. Полное покрытие AVM1,
оптимизирующий компилятор и завершённая платформенная приёмка пока не заявлены.

| Архитектура | Реализованный OS ABI | Проверка здесь |
| --- | --- | --- |
| x86 | i386 System V, cdecl, 32 бита | Исполнение машинных блоков в Unicorn |
| x86-64 | Linux System V AMD64, 64 бита | Полная VM и примеры на текущей машине |
| macOS ARM64 (opt-in) | Darwin C, X29 frame chain, macOS 11.4+ MAP_JIT callback | Unicorn и controlled SDK/policy/thread fixtures; включение по умолчанию OFF, нативная Apple-приёмка открыта |
| macOS x64 | Darwin C, цепочка RBP | Linux execution/AVM1 pipeline и SDK fixtures; нативная приёмка открыта |
| Windows x64 | Microsoft x64, 64 бита | Реальные ms_abi-вызовы на Linux и SDK protocol fixtures; Windows OS/unwind приёмка ещё не выполнена |
| ARM | ARMv7 A32, AAPCS32 | Unicorn, включая вызовы Thumb-обработчиков |
| ARM64 | AArch64, AAPCS64, LP64 | Unicorn, включая адреса выше 4 ГБ |
| RISC-V | RV32I, стандартный ILP32 psABI | Unicorn |
| RISC-V64 | RV64I, стандартный LP64 psABI | Unicorn, включая адреса выше 4 ГБ |

Все генераторы используют little-endian. ARM до v7 и профиль M, RV32E/RV64E,
бигендианные варианты, x32 и AArch64 ILP32 исключены. Генераторы RISC-V не
требуют расширения сжатых инструкций; сами обработчики собираются выбранным
системным компилятором. Указатели на обработчики сохраняются полностью.

Для новых архитектур проверены аргументы, сохранение регистров, выравнивание
стека, прямые и обратные переходы, дальние переходы за пределы 1 МБ, возврат
статуса ошибки. Это проверка машинных блоков с синтетическими адресами
обработчиков. Полная VM на чужой процессорной архитектуре здесь ещё не
собиралась и не запускалась. Стадии 8 и 9 дорожной карты остаются открытыми.

Linux и Intel macOS сначала подготавливают RW-память, затем переводят её в RX
и синхронизируют кеш инструкций. Включаемый ARM64 macOS-порт вместо mprotect
использует один MAP_JIT-пул с per-thread W^X через системный allowlisted callback;
его mmap-запрос содержит RWX, а одновременный доступ записи/исполнения исключает
защита текущего потока. См. [JIT_MACOS_ARM64.md](JIT_MACOS_ARM64.md). Запрет исполняемой памяти со стороны ОС приводит к UNSUPPORTED при
компиляции; автоматический режим допускает интерпретацию.

## Запуск интерпретации и JIT

Передайте [`FVM_ExecutionOptions`](../include/flashvm/execution_mode.h#L20) в [`fvm_function_invoke_options`](../include/flashvm/execution_mode.h#L76) или
[`fvm_avm1_execute_options`](../include/flashvm/execution_mode.h#L73). Инициализируйте структуру через
[`fvm_execution_options_init`](../include/flashvm/execution_mode.h#L61). Поле `mode` принимает:

- [`FVM_EXECUTION_INTERPRETER`](../include/flashvm/execution_mode.h#L14): исполнять исходное тело интерпретатором;
- [`FVM_EXECUTION_JIT`](../include/flashvm/execution_mode.h#L15): скомпилировать поддерживаемое тело и кешировать его
  внутри контекста; иначе использовать интерпретатор;
- [`FVM_EXECUTION_MANUAL`](../include/flashvm/execution_mode.h#L13): сохранить старую семантику ручных привязок native body.

`fvm_execution_set_mode(context, mode, flags)` меняет политику следующих и
вложенных вызовов. Текущий машинный кадр завершается прежним способом;
переписывания его стека и переключения посреди инструкции нет.

Архитектура выбирается при сборке FlashVM. `fvm_jit_backend_name()` возвращает
её имя, а `fvm_jit_available()` сообщает о наличии собранного backend.
Публичный API не компилирует код для чужого CPU внутри текущего процесса.

Готовые C89 примеры:

| Пример | Что показывает |
| --- | --- |
| `examples/embedding/execution_modes/main.c` | Интерпретация, автоматический JIT, переключение и диагностический replay |
| `examples/embedding/jit/main.c` | Ручная компиляция, привязка и снятие native body |
| `examples/embedding/jit_debug/main.c` | Breakpoint, выполнение тела интерпретатором, возврат к JIT |
| `examples/embedding/jit_workers/main.c` | Два OS worker-потока, у каждого собственная VM и кеш |
| `examples/embedding/jit_instances/main.c` | Две одновременно существующие VM в одном процессе с разными режимами |

В этих примерах ошибки проверяются через `if`, в том числе в Release без
assertions. При частичной инициализации ресурсы освобождаются; созданные worker
потоки сначала присоединяются через join, затем уничтожаются их дескрипторы.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
./build/flashvm_examples_embedding_execution_modes_main
./build/flashvm_examples_embedding_jit_debug_main
./build/flashvm_examples_embedding_jit_workers_main
./build/flashvm_examples_embedding_jit_instances_main
```

Один и тот же исходник примера применяется для всех архитектур. Для чужой
архитектуры нужен Linux cross-toolchain с соответствующим sysroot. Пары
компиляторов и пример CMake для ARM64 приведены в
[JIT_ARCHITECTURES.md](JIT_ARCHITECTURES.md#build-and-run-the-same-examples).
Компилятор для bare metal вместо Linux не подходит. Для RV32 необходим именно
Linux ILP32 sysroot; наличие riscv64-компилятора само по себе его не гарантирует.

DEFS [`INTERPRETER`](../cmake/BuildDefinitions.cmake#L13) включает библиотеку с переключаемыми runtime-режимами.
Пресет DEFS [`JIT`](../cmake/BuildDefinitions.cmake#L13) включает автоматическую компиляцию с fallback для новых контекстов. Пример потоков
требует OS ports; в NO_OS_PORTS он явно сообщает об их отсутствии.

## Breakpoint и встроенный отладочный интерфейс

`flashvm/debug.h` объединяет существующие сведения о source/line/offset и новые
операции установки breakpoint:

- [`fvm_debug_map_action`](../include/flashvm/debug.h#L63): связать имя функции и AVM1 offset с файлом/строкой;
- [`fvm_debug_set_breakpoint`](../include/flashvm/debug.h#L51): включить или удалить точную позицию инструкции;
- [`fvm_debug_set_breakpoint_handler`](../include/flashvm/debug.h#L54): установить callback хоста.

Само добавление source map не устанавливает breakpoint. Для остановки по строке
найдите соответствующий action offset и установите его. Callback вызывается
перед этой инструкцией после обновления отладочной позиции и проверки бюджета.
Возврат [FVM_OK](../include/flashvm/flashvm.h#L42) продолжает выполнение, другой статус прерывает текущий вызов.

Наличие хотя бы одной точки переводит **всё тело соответствующей функции или
блока** в интерпретацию, даже если код уже находится в JIT-кеше или вручную
привязан через [`fvm_jit_body`](../include/flashvm/jit.h#L24). Другие функции могут работать через JIT.
После удаления последней точки следующий вход снова использует кеш.
Без callback тело всё равно интерпретируется.

Идентификация сейчас по имени функции: одноимённые тела разделяют точки.
Корневые блоки используют пустую строку, а анонимные функции, созданные C API,
имя `<function>`. Это существующие имена отладочных кадров FlashVM.
Исходный файл и адрес объекта функции пока не уточняют выбор breakpoint.

Точки нужно устанавливать в потоке владельца VM, обычно перед вызовом или
при остановке. Если машинный кадр уже выполняется, новая точка влияет на
последующие входы, а не перестраивает активный кадр. Callback синхронный:
хост может ждать в нём команд своего debugger UI. Другой поток общается с ним
через синхронизацию хоста и не меняет контекст напрямую. Указатели в записи
позиции действуют только до возврата callback. Не запускайте VM повторно,
не вызывайте GC, не уничтожайте контекст и не выбрасывайте C++ исключения
через C callback. Можно менять breakpoint и режим исполнения.

Интерфейс обеспечивает остановку/продолжение средствами хоста, а не готовое
графическое окно отладчика и не пошаговую отладку машинных инструкций.
Пользовательские native bridges остаются C callbacks; подменяется только JIT,
для которого сохранено интерпретируемое исходное тело.

## Исключение в JIT и диагностический повтор

По умолчанию исключение просто передаётся вызывающему коду. Флаг
[`FVM_EXEC_REPLAY_ON_EXCEPTION`](../include/flashvm/execution_mode.h#L17) явно разрешает повтор исходной функции через
интерпретатор с журналом и статистикой. Повтор получает свежую активацию,
а после него наружу возвращается первоначальное исключение, даже если повтор
завершился успешно. Побочные эффекты первого прохода не откатываются и могут
повториться. Поэтому этот режим предназначен для разрешённой хостом диагностики,
а не для незаметного восстановления состояния. Ошибки CPU и произвольные
сигналы не превращаются в безопасный replay. Подробности —
[EXECUTION_MODES.md](EXECUTION_MODES.md).

## Потоки и несколько VM

Каждый контекст владеет своим режимом, автоматическим кешем, breakpoint,
статистикой и logger. Глобального изменяемого JIT-кеша или режима нет.
Два worker могут одновременно компилировать и выполнять код, если каждый
владеет отдельной VM. Один контекст нельзя одновременно вызывать из двух
потоков. Ручные [FVM_JitCode](../src/jit/internal.hpp#L59) имеют несинхронизированный счётчик активности:
не разделяйте один дескриптор между параллельными потоками, компилируйте отдельно.
Автоматический кеш уже разделён по контекстам.

Пример `jit_workers` запускает оба worker до ожидания результатов. Каждый
проверяет вход в native body, если backend доступен, и возвращает только число.
Результаты 42 и 44 проверяются после join. В минимальной сборке без OS ports
параллельное выполнение не заявляется.

Пример `jit_instances` одновременно держит две пары runtime/context. Первая
интерпретирует, вторая запрашивает JIT; результат обеих равен 42, но счётчики
native entries независимы. Для параллельного выполнения назначьте каждой паре
свой поток, как в worker-примере. Нельзя передавать managed-объекты между
контекстами обычным копированием указателей. Используйте предусмотренный
графовый перенос; привязки машинного кода при таком переносе не сохраняются.
Уничтожайте контексты перед их runtime: это освобождает принадлежащие им кеши.

## Повторный прогон кодировщиков v0.359

В Unicorn 2.1.4 повторно прошли все 26 текущих сценариев ABI/переходов: x86,
ARM, ARM64, RV32 и RV64; обычные/дальние переходы, вызовы Thumb из ARM, высокие
64-битные адреса helper и успешный/ошибочный выход. Векторы созданы текущей
минимальной Release-сборкой и исполнены scripts/check_jit_backends.py. Это
исполнение сгенерированных инструкций с заглушками helper; полная VM на другой
CPU/ОС таким прогоном не проверяется.


Windows x64 offline call-plan encoder and actual Linux `ms_abi` validation / автономный генератор и проверка ABI: [JIT_WINDOWS_X64.md](JIT_WINDOWS_X64.md). v0.561 adds Windows x64 selection, W^X allocation and registered unwind metadata; native Windows execution is still unverified. / Добавлены выбор backend, W^X и unwind; нативный Windows запуск ещё не проверен.


macOS Intel offline encoder and linked-frame validation / автономный генератор и цепочка кадров: [JIT_MACOS_X64.md](JIT_MACOS_X64.md). v0.564 adds classic desktop macOS x64 RW→RX publication and explicit signing preparation; native macOS acceptance remains pending / добавлены classic RW→RX и подготовка подписи, нативная приёмка ещё не выполнена.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_avm1_execute_options` | function | [include/flashvm/execution_mode.h:73](../include/flashvm/execution_mode.h#L73) |
| `fvm_debug_map_action` | function | [include/flashvm/debug.h:63](../include/flashvm/debug.h#L63) |
| `fvm_debug_set_breakpoint` | function | [include/flashvm/debug.h:51](../include/flashvm/debug.h#L51) |
| `fvm_debug_set_breakpoint_handler` | function | [include/flashvm/debug.h:54](../include/flashvm/debug.h#L54) |
| `FVM_EXEC_REPLAY_ON_EXCEPTION` | macro / define | [include/flashvm/execution_mode.h:17](../include/flashvm/execution_mode.h#L17) |
| `FVM_EXECUTION_INTERPRETER` | enum constant | [include/flashvm/execution_mode.h:14](../include/flashvm/execution_mode.h#L14) |
| `FVM_EXECUTION_JIT` | enum constant | [include/flashvm/execution_mode.h:15](../include/flashvm/execution_mode.h#L15) |
| `FVM_EXECUTION_MANUAL` | enum constant | [include/flashvm/execution_mode.h:13](../include/flashvm/execution_mode.h#L13) |
| `fvm_execution_options_init` | function | [include/flashvm/execution_mode.h:61](../include/flashvm/execution_mode.h#L61) |
| `fvm_execution_set_mode` | function | [include/flashvm/execution_mode.h:64](../include/flashvm/execution_mode.h#L64) |
| `FVM_ExecutionOptions` | struct | [include/flashvm/execution_mode.h:20](../include/flashvm/execution_mode.h#L20) |
| `fvm_function_invoke_options` | function | [include/flashvm/execution_mode.h:76](../include/flashvm/execution_mode.h#L76) |
| `fvm_jit_available` | function | [include/flashvm/jit.h:13](../include/flashvm/jit.h#L13) |
| `fvm_jit_backend_name` | function | [include/flashvm/jit.h:15](../include/flashvm/jit.h#L15) |
| `fvm_jit_body` | function | [include/flashvm/jit.h:24](../include/flashvm/jit.h#L24) |
| `FVM_JitCode` | struct | [src/jit/internal.hpp:59](../src/jit/internal.hpp#L59) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `INTERPRETER` | DEFS switch | [cmake/BuildDefinitions.cmake:13](../cmake/BuildDefinitions.cmake#L13) |
| `JIT` | DEFS switch | [cmake/BuildDefinitions.cmake:13](../cmake/BuildDefinitions.cmake#L13) |
<!-- source-links:end -->
