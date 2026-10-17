# Перехват сообщений канала / channel inspection

Показывает полезный debugger-сценарий для очередей: остановить доставку, посмотреть снимок сообщения и затем продолжить обычную работу.

## Что происходит

Создаётся локальная FIFO, в неё без блокировки отправляется число 42. Freeze запрещает delivery, но не enqueue. Снимок metadata показывает pending/frozen; debug_channel_message декодирует независимое значение без dequeue.

## Реализация и время жизни

Все receive здесь неблокирующие. Во время freeze ожидается NOT_FOUND, чтобы пример не мог зависнуть. После resume обычный receive получает исходное число. Канал уничтожается только после возврата всех его синхронных пользователей, а значения — вместе с контекстом.

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
cmake --build build-debugger --target flashvm_examples_debugger_channel_inspection_main --parallel
./build-debugger/flashvm_examples_debugger_channel_inspection_main
```

## Ожидаемый результат

Печатаются идентичность очереди, pending=1, frozen=1 и inspected value 42. После resume проверяется фактический тип/значение полученного сообщения обычной веткой ошибки.

## Ограничения и особенности

Заморожена доставка очереди, а не весь процесс или содержимое live resource capabilities. При просмотре объектных графов результаты нужно pin перед дальнейшим GC. Для remote/file существуют отдельные transport/scan ограничения; пример намеренно использует локальный примитив.

NATURAL/FORCE — политика VM, независимая от CMake Release/Debug или host
компиляторных -g/-O0. Host callbacks и borrowed pointers принадлежат указанным
в API владельцам. Любая неожиданная ошибка даёт ненулевой exit code.

## Используемые функции

- [`fvm_channel_create`](../../../include/flashvm/channel.h) — декларация и контракт общего VM API.
- [`fvm_channel_destroy`](../../../include/flashvm/channel.h) — декларация и контракт общего VM API.
- [`fvm_channel_receive_value`](../../../include/flashvm/transfer.h) — декларация и контракт общего VM API.
- [`fvm_channel_send_value`](../../../include/flashvm/transfer.h) — декларация и контракт общего VM API.
- [`fvm_debug_channel_freeze`](../../../doc/DEBUG_API.md#fvm_debug_channel_freeze) — подробный debugger/runtime-контракт.
- [`fvm_debug_channel_message`](../../../doc/DEBUG_API.md#fvm_debug_channel_message) — подробный debugger/runtime-контракт.
- [`fvm_debug_channel_state`](../../../doc/DEBUG_API.md#fvm_debug_channel_state) — подробный debugger/runtime-контракт.
- [`fvm_value_number`](../../../include/flashvm/value.h) — декларация и контракт общего VM API.

[Полный справочник EN](../../../doc/DEBUG_API.md) ·
[Справочник RU](../../../doc/DEBUG_API.ru.md) ·
[Политики сбоев](../../../doc/CRASH_DIAGNOSTICS.md) ·
[SWF/SWD](../../../doc/SWD_DEBUGGING.md).

## English

This C89 tutorial demonstrates **channel inspection**. It uses explicit
error paths and line-by-line comments, with no process assertions. Build/run
commands above select the minimal profile. The implementation/lifetime notes
explain the expected VM result, borrowed data and cleanup; every used FlashVM
function links to its detailed reference or public ownership contract.
