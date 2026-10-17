# Сохранённый бэктрейс / retained backtrace

Позволяет приложению получить отчёт уже после возврата из аварийного AVM1-вызова. Подходит для headless-обработки ошибок, логирования и последующего интерфейса отладчика.

## Что происходит

Создаётся обычная AVM1-функция с параметром x. Ей передаётся 42, а её байткод выполняет GetVariable и Throw. Программа запрашивает автоматический JIT, но сохраняет interpreter fallback на неподдерживаемом хосте. Режим BACKTRACE работает с обычным NATURAL и не копирует входные значения на каждом вызове.

## Реализация и время жизни

После ожидаемого FVM_EXCEPTION извлекается JSON через fvm_crash_data(..., 0). Он пишется с явной длиной через fwrite: буфер не обязан завершаться NUL. Отчёт принадлежит контексту, поэтому его представление используется до debugger_close. Затем намеренное исключение очищается.

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
cmake --build build-debugger --target flashvm_examples_debugger_backtrace_main --parallel
./build-debugger/flashvm_examples_debugger_backtrace_main
```

## Ожидаемый результат

Выводится JSON с именем функции, фактическим engine, аргументом 42, AVM1-смещением и исходными метаданными. Host exit code 0 означает, что намеренный script throw успешно обработан; это не успешный результат самого скрипта.

## Ограничения и особенности

Источник текста не подключён, поэтому source/line могут оставаться неизвестными. Объектные аргументы представлены диагностическими идентичностями; подробные поля объектов доступны в ZIP. BACKTRACE не перехватывает SIGSEGV/SEH.

NATURAL/FORCE — политика VM, независимая от CMake Release/Debug или host
компиляторных -g/-O0. Host callbacks и borrowed pointers принадлежат указанным
в API владельцам. Любая неожиданная ошибка даёт ненулевой exit code.

## Используемые функции

- [`fvm_crash_configure`](../../../doc/DEBUG_API.md#fvm_crash_configure) — подробный debugger/runtime-контракт.
- [`fvm_crash_data`](../../../doc/DEBUG_API.md#fvm_crash_data) — подробный debugger/runtime-контракт.
- [`fvm_exception_clear`](../../../include/flashvm/exception.h) — декларация и контракт общего VM API.
- [`fvm_execution_set_mode`](../../../doc/DEBUG_API.md#fvm_execution_set_mode) — подробный debugger/runtime-контракт.
- [`fvm_function_create_avm1`](../../../include/flashvm/function.h) — декларация и контракт общего VM API.
- [`fvm_function_invoke`](../../../include/flashvm/function.h) — декларация и контракт общего VM API.
- [`fvm_value_number`](../../../include/flashvm/value.h) — декларация и контракт общего VM API.

[Полный справочник EN](../../../doc/DEBUG_API.md) ·
[Справочник RU](../../../doc/DEBUG_API.ru.md) ·
[Политики сбоев](../../../doc/CRASH_DIAGNOSTICS.md) ·
[SWF/SWD](../../../doc/SWD_DEBUGGING.md).

## English

This C89 tutorial demonstrates **retained backtrace**. It uses explicit
error paths and line-by-line comments, with no process assertions. Build/run
commands above select the minimal profile. The implementation/lifetime notes
explain the expected VM result, borrowed data and cleanup; every used FlashVM
function links to its detailed reference or public ownership contract.
