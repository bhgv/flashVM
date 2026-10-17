# SWF/SWD и текущая строка / source breakpoint

Показывает, как интерфейс отладчика получает настоящее имя файла, исходный текст и строку выполнявшегося Action-кода из matching SWD.

## Что происходит

fixture.h содержит маленький независимый FWS8/FWD8 пример: 16-байтный DebugID, исходник Main.as, один line record и AVM1 Throw. load_swd проверяет совпадение идентификаторов и копирует метаданные. Во внешнем приложении вместо массивов передаются буферы настоящих SWF/SWD-файлов; API сам файлы не открывает.

## Реализация и время жизни

Запрошен JIT, но точный breakpoint корневого тела на offset 8 исключает это тело из native на следующем входе. show_line получает имя/строку, вызывает current_source и показывает её содержимое. Crash action остаётся NONE: source debugger не требует ZIP или обработки ошибок. После callback интерпретатор продолжает Throw.

Общие helpers [`../support.h`](../support.h) создают isolated runtime/context,
устанавливают стандартные классы, печатают точные коды ошибок и освобождают
контекст раньше runtime. Их setup/cleanup APIs перечислены в
[общем README](../README.md#общая-инициализация-и-ошибки). Каждая непустая строка
C/header прокомментирована; проверки выполняются обычным control flow.

## Сборка и запуск

Из корня репозитория:

```sh
cmake -S . -B build-debugger -DFVM_DEFS_FILE="$PWD/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-debugger --target flashvm_examples_debugger_source_breakpoints_main --parallel
./build-debugger/flashvm_examples_debugger_source_breakpoints_main
```

## Ожидаемый результат

Печатается Main.as:2 и текст throw 42;. Программа требует ровно одну остановку, явно обрабатывает намеренное исключение и удаляет символы вне исполнения.

## Ограничения и особенности

SWD6–8, FWS/CWS поддерживаются с указанными в справочнике ограничениями. Одинаковые тела с конфликтующими строками возвращают NOT_FOUND, а не чужой исходник. Breakpoint не патчит текущий native frame. Эта fixture демонстрирует формат и API, но не заменяет проверку каждого реального authoring tool.

NATURAL/FORCE — политика VM, независимая от CMake Release/Debug или host
компиляторных -g/-O0. Host callbacks и borrowed pointers принадлежат указанным
в API владельцам. Любая неожиданная ошибка даёт ненулевой exit code.

## Используемые функции

- [`fvm_avm1_execute`](../../../include/flashvm/avm1.h) — декларация и контракт общего VM API.
- [`fvm_debug_clear_symbols`](../../../doc/DEBUG_API.md#fvm_debug_clear_symbols) — подробный debugger/runtime-контракт.
- [`fvm_debug_current_source`](../../../doc/DEBUG_API.md#fvm_debug_current_source) — подробный debugger/runtime-контракт.
- [`fvm_debug_load_swd`](../../../doc/DEBUG_API.md#fvm_debug_load_swd) — подробный debugger/runtime-контракт.
- [`fvm_debug_set_breakpoint`](../../../doc/DEBUG_API.md#fvm_debug_set_breakpoint) — подробный debugger/runtime-контракт.
- [`fvm_debug_set_breakpoint_handler`](../../../doc/DEBUG_API.md#fvm_debug_set_breakpoint_handler) — подробный debugger/runtime-контракт.
- [`fvm_exception_clear`](../../../include/flashvm/exception.h) — декларация и контракт общего VM API.
- [`fvm_execution_set_mode`](../../../doc/DEBUG_API.md#fvm_execution_set_mode) — подробный debugger/runtime-контракт.

[Полный справочник EN](../../../doc/DEBUG_API.md) ·
[Справочник RU](../../../doc/DEBUG_API.ru.md) ·
[Политики сбоев](../../../doc/CRASH_DIAGNOSTICS.md) ·
[SWF/SWD](../../../doc/SWD_DEBUGGING.md).

## English

This C89 tutorial demonstrates **source breakpoint**. It uses explicit
error paths and line-by-line comments, with no process assertions. Build/run
commands above select the minimal profile. The implementation/lifetime notes
explain the expected VM result, borrowed data and cleanup; every used FlashVM
function links to its detailed reference or public ownership contract.

<!-- example-catalog-api:start -->
## Назначение и особенности

Подключает согласованную пару SWF/SWD, ставит breakpoint по смещению и показывает Main.as с текущей строкой. Выбранное тело исполняется интерпретатором; crash action NONE не мешает осмотру исходника.

**Текущая CMake-цель:** `flashvm_examples_debugger_source_breakpoints_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_avm1_execute`](../../../include/flashvm/avm1.h) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — AVM1-вызовы и связь с host API. |
| [`fvm_debug_clear_symbols`](../../../include/flashvm/source_debug.h) | [DEBUG_API.md#fvm_debug_clear_symbols](../../../doc/DEBUG_API.md#fvm_debug_clear_symbols) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_debug_current_source`](../../../include/flashvm/source_debug.h) | [DEBUG_API.md#fvm_debug_current_source](../../../doc/DEBUG_API.md#fvm_debug_current_source) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_debug_load_swd`](../../../include/flashvm/source_debug.h) | [DEBUG_API.md#fvm_debug_load_swd](../../../doc/DEBUG_API.md#fvm_debug_load_swd) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_debug_set_breakpoint`](../../../include/flashvm/debug.h) | [DEBUG_API.md#fvm_debug_set_breakpoint](../../../doc/DEBUG_API.md#fvm_debug_set_breakpoint) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_debug_set_breakpoint_handler`](../../../include/flashvm/debug.h) | [DEBUG_API.md#fvm_debug_set_breakpoint_handler](../../../doc/DEBUG_API.md#fvm_debug_set_breakpoint_handler) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_exception_clear`](../../../include/flashvm/exception.h) | [STANDARD_ERROR.md](../../../doc/STANDARD_ERROR.md) — Ошибки и сохранение исходного исключения. |
| [`fvm_execution_set_mode`](../../../include/flashvm/execution_mode.h) | [DEBUG_API.md#fvm_execution_set_mode](../../../doc/DEBUG_API.md#fvm_execution_set_mode) — Параметры, статусы, время жизни и ограничения вызова. |

[Описание в общем каталоге](../../README.md#example-debugger-source_breakpoints).
<!-- example-catalog-api:end -->
