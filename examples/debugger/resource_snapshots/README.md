# Реестры VM и владение mutex / resource snapshots

Демонстрирует инспекцию объектов/функций VM и согласованный снимок native mutex registry без искусственного deadlock или блокирующего ожидания.

## Что происходит

Сначала считаются объекты, глобальные native-функции и именованные script registry entries; counted-вариант имени сохраняет полную byte identity. Затем один мьютекс захватывается неблокирующим вызовом с идентичностью данного контекста.

## Реализация и время жизни

Capacity сначала запрашивается через null/zero probe. NOT_FOUND при probe означает необходимость выделить нужное число строк, а не ошибку мьютекса. Проверяется переполнение malloc-размера, выполняется точное копирование строк, затем ownership снимается тем же контекстом. При росте реестра между probe/copy API возвращает NOT_FOUND; этот короткий пример сообщает его, а production debugger может повторить ограниченное число раз.

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
cmake --build build-debugger --target flashvm_examples_debugger_resource_snapshots_main --parallel
./build-debugger/flashvm_examples_debugger_resource_snapshots_main
```

## Ожидаемый результат

Печатаются размеры реестров, первое полное native-имя при наличии и скопированные owner/waiter/deadlock/remote поля мьютексов. Именованный script registry может быть пустым: это не реестр всех heap-замыканий.

## Ограничения и особенности

Поля deadlocked описывают локальный wait graph. Remote endpoint/address — наблюдение proxy, а не доказательство распределённого цикла. Строковые поля снимков ограничены размерами struct. Нужен OS_PORTS для native synchronization; при неподдерживаемом порте проверка вернёт обычную ошибку.

NATURAL/FORCE — политика VM, независимая от CMake Release/Debug или host
компиляторных -g/-O0. Host callbacks и borrowed pointers принадлежат указанным
в API владельцам. Любая неожиданная ошибка даёт ненулевой exit code.

## Используемые функции

- [`fvm_debug_mutex_detail_snapshot`](../../../doc/DEBUG_API.md#fvm_debug_mutex_detail_snapshot) — подробный debugger/runtime-контракт.
- [`fvm_debug_native_function_at_bytes`](../../../doc/DEBUG_API.md#fvm_debug_native_function_at_bytes) — подробный debugger/runtime-контракт.
- [`fvm_debug_native_function_count`](../../../doc/DEBUG_API.md#fvm_debug_native_function_count) — подробный debugger/runtime-контракт.
- [`fvm_debug_object_count`](../../../doc/DEBUG_API.md#fvm_debug_object_count) — подробный debugger/runtime-контракт.
- [`fvm_debug_script_function_count`](../../../doc/DEBUG_API.md#fvm_debug_script_function_count) — подробный debugger/runtime-контракт.
- [`fvm_mutex_create`](../../../include/flashvm/mutex.h) — декларация и контракт общего VM API.
- [`fvm_mutex_release`](../../../include/flashvm/mutex.h) — декларация и контракт общего VM API.
- [`fvm_mutex_reset`](../../../include/flashvm/mutex.h) — декларация и контракт общего VM API.
- [`fvm_mutex_set`](../../../include/flashvm/mutex.h) — декларация и контракт общего VM API.

[Полный справочник EN](../../../doc/DEBUG_API.md) ·
[Справочник RU](../../../doc/DEBUG_API.ru.md) ·
[Политики сбоев](../../../doc/CRASH_DIAGNOSTICS.md) ·
[SWF/SWD](../../../doc/SWD_DEBUGGING.md).

## English

This C89 tutorial demonstrates **resource snapshots**. It uses explicit
error paths and line-by-line comments, with no process assertions. Build/run
commands above select the minimal profile. The implementation/lifetime notes
explain the expected VM result, borrowed data and cleanup; every used FlashVM
function links to its detailed reference or public ownership contract.

<!-- example-catalog-api:start -->
## Назначение и особенности

Перечисляет объекты и реестры функций, затем снимает согласованный снимок владельцев mutex. Показывает двухшаговый запрос размера и копирование данных, не создавая искусственный deadlock.

**Текущая CMake-цель:** `flashvm_examples_debugger_resource_snapshots_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_debug_mutex_detail_snapshot`](../../../include/flashvm/debug.h) | [DEBUG_API.md#fvm_debug_mutex_detail_snapshot](../../../doc/DEBUG_API.md#fvm_debug_mutex_detail_snapshot) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_debug_native_function_at_bytes`](../../../include/flashvm/debug.h) | [DEBUG_API.md#fvm_debug_native_function_at_bytes](../../../doc/DEBUG_API.md#fvm_debug_native_function_at_bytes) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_debug_native_function_count`](../../../include/flashvm/debug.h) | [DEBUG_API.md#fvm_debug_native_function_count](../../../doc/DEBUG_API.md#fvm_debug_native_function_count) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_debug_object_count`](../../../include/flashvm/debug.h) | [DEBUG_API.md#fvm_debug_object_count](../../../doc/DEBUG_API.md#fvm_debug_object_count) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_debug_script_function_count`](../../../include/flashvm/debug.h) | [DEBUG_API.md#fvm_debug_script_function_count](../../../doc/DEBUG_API.md#fvm_debug_script_function_count) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_mutex_create`](../../../include/flashvm/mutex.h) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_mutex_release`](../../../include/flashvm/mutex.h) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_mutex_reset`](../../../include/flashvm/mutex.h) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |
| [`fvm_mutex_set`](../../../include/flashvm/mutex.h) | [MUTEX_CHECKPOINT.md](../../../doc/MUTEX_CHECKPOINT.md) — Идентичность mutex, захват и освобождение. |

[Описание в общем каталоге](../../README.md#example-debugger-resource_snapshots).
<!-- example-catalog-api:end -->
