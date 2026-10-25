# Живые переменные и FORCE / live variables

Показывает остановку до раскрутки активаций: debugger API ещё может прочитать исходные аргументы, материализованные локальные значения и ABI-регистры.

## Что происходит

Действие DEBUGGER и детализация FORCE задаются независимо от [FVM_EXECUTION_JIT](../../../include/flashvm/execution_mode.h#L15). FORCE сохраняет входные argument cells и текст строк, а текущий JIT продолжает исполнять настоящий машинный код. У данного backend локальные значения уже находятся в activation/register-структурах; дополнительный dump на каждую инструкцию не создаётся.

## Реализация и время жизни

Host-owned callback получает указатель на целочисленный результат инспекции. Он читает аргумент нулевого кадра, перечисляет доступные локальные значения до NOT_FOUND и пробует существующий регистр ноль. Callback не выполняет скрипт, не вызывает GC, не изменяет и не уничтожает VM. Возврат продолжает обычную обработку исходного Throw.

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
cmake --build build-debugger --target flashvm_examples_debugger_live_variables_main --parallel
./build-debugger/flashvm_examples_debugger_live_variables_main
```

## Ожидаемый результат

Печатаются причина остановки, исходный аргумент 42, имена/теги доступных локальных значений и доступность регистра. После выхода callback main проверяет ожидаемое исключение и результат инспекции.

## Ограничения и особенности

Заимствованные значения callback недействительны после его возврата. Для паузы GUI хост может блокироваться внутри callback и выполнять read-only запросы на VM-потоке. Возврат не повторяет аварийную инструкцию. FORCE не делает глубокую копию объектного графа на входе.

NATURAL/FORCE — политика VM, независимая от CMake Release/Debug или host
компиляторных -g/-O0. Host callbacks и borrowed pointers принадлежат указанным
в API владельцам. Любая неожиданная ошибка даёт ненулевой exit code.

## Используемые функции

- [`fvm_crash_argument`](../../../doc/DEBUG_API.md#fvm_crash_argument) — подробный debugger/runtime-контракт.
- [`fvm_crash_configure`](../../../doc/DEBUG_API.md#fvm_crash_configure) — подробный debugger/runtime-контракт.
- [`fvm_crash_local`](../../../doc/DEBUG_API.md#fvm_crash_local) — подробный debugger/runtime-контракт.
- [`fvm_crash_register`](../../../doc/DEBUG_API.md#fvm_crash_register) — подробный debugger/runtime-контракт.
- [`fvm_crash_set_detail`](../../../doc/DEBUG_API.md#fvm_crash_set_detail) — подробный debugger/runtime-контракт.
- [`fvm_debug_frame_at`](../../../doc/DEBUG_API.md#fvm_debug_frame_at) — подробный debugger/runtime-контракт.
- [`fvm_exception_clear`](../../../include/flashvm/exception.h#L74) — декларация и контракт общего VM API.
- [`fvm_execution_set_mode`](../../../doc/DEBUG_API.md#fvm_execution_set_mode) — подробный debugger/runtime-контракт.
- [`fvm_function_create_avm1`](../../../include/flashvm/function.h#L62) — декларация и контракт общего VM API.
- [`fvm_function_invoke`](../../../include/flashvm/function.h#L79) — декларация и контракт общего VM API.
- [`fvm_value_number`](../../../include/flashvm/value.h#L50) — декларация и контракт общего VM API.

[Полный справочник EN](../../../doc/DEBUG_API.md) ·
[Справочник RU](../../../doc/DEBUG_API.ru.md) ·
[Политики сбоев](../../../doc/CRASH_DIAGNOSTICS.md) ·
[SWF/SWD](../../../doc/SWD_DEBUGGING.md).

## English

This C89 tutorial demonstrates **live variables**. It uses explicit
error paths and line-by-line comments, with no process assertions. Build/run
commands above select the minimal profile. The implementation/lifetime notes
explain the expected VM result, borrowed data and cleanup; every used FlashVM
function links to its detailed reference or public ownership contract.

<!-- example-catalog-api:start -->
## Назначение и особенности

Входит в синхронный debugger callback до раскрутки кадров и читает аргумент, локальные значения и регистр. FORCE и выбор JIT независимы; callback проверяет успешность сохранения и возвращает управление обычной обработке ошибки.

**Текущая CMake-цель:** `flashvm_examples_debugger_live_variables_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_crash_argument`](../../../include/flashvm/crash.h#L103) | [DEBUG_API.md#fvm_crash_argument](../../../doc/DEBUG_API.md#fvm_crash_argument) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_crash_configure`](../../../include/flashvm/crash.h#L65) | [DEBUG_API.md#fvm_crash_configure](../../../doc/DEBUG_API.md#fvm_crash_configure) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_crash_local`](../../../include/flashvm/crash.h#L107) | [DEBUG_API.md#fvm_crash_local](../../../doc/DEBUG_API.md#fvm_crash_local) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_crash_register`](../../../include/flashvm/crash.h#L112) | [DEBUG_API.md#fvm_crash_register](../../../doc/DEBUG_API.md#fvm_crash_register) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_crash_set_detail`](../../../include/flashvm/crash.h#L72) | [DEBUG_API.md#fvm_crash_set_detail](../../../doc/DEBUG_API.md#fvm_crash_set_detail) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_debug_frame_at`](../../../include/flashvm/debug.h#L38) | [DEBUG_API.md#fvm_debug_frame_at](../../../doc/DEBUG_API.md#fvm_debug_frame_at) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_exception_clear`](../../../include/flashvm/exception.h#L74) | [STANDARD_ERROR.md](../../../doc/STANDARD_ERROR.md) — Ошибки и сохранение исходного исключения. |
| [`fvm_execution_set_mode`](../../../include/flashvm/execution_mode.h#L79) | [DEBUG_API.md#fvm_execution_set_mode](../../../doc/DEBUG_API.md#fvm_execution_set_mode) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_function_create_avm1`](../../../include/flashvm/function.h#L62) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_function_invoke`](../../../include/flashvm/function.h#L79) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Создание функций, аргументы, вызов и receiver. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L50) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-debugger-live_variables).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_crash_argument` | function | [include/flashvm/crash.h:103](../../../include/flashvm/crash.h#L103) |
| `fvm_crash_configure` | function | [include/flashvm/crash.h:65](../../../include/flashvm/crash.h#L65) |
| `fvm_crash_local` | function | [include/flashvm/crash.h:107](../../../include/flashvm/crash.h#L107) |
| `fvm_crash_register` | function | [include/flashvm/crash.h:112](../../../include/flashvm/crash.h#L112) |
| `fvm_crash_set_detail` | function | [include/flashvm/crash.h:72](../../../include/flashvm/crash.h#L72) |
| `fvm_debug_frame_at` | function | [include/flashvm/debug.h:38](../../../include/flashvm/debug.h#L38) |
| `fvm_exception_clear` | function | [include/flashvm/exception.h:74](../../../include/flashvm/exception.h#L74) |
| `FVM_EXECUTION_JIT` | enum constant | [include/flashvm/execution_mode.h:15](../../../include/flashvm/execution_mode.h#L15) |
| `fvm_execution_set_mode` | function | [include/flashvm/execution_mode.h:79](../../../include/flashvm/execution_mode.h#L79) |
| `fvm_function_create_avm1` | function | [include/flashvm/function.h:62](../../../include/flashvm/function.h#L62) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:79](../../../include/flashvm/function.h#L79) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
<!-- source-links:end -->
