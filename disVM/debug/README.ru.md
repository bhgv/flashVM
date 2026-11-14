# Reader-модули диагностики resident

Copyright (c) 2026 bhgv

[English](README.md)

`residentdebug.b` реализует `module/residentdebug.m`: владеет одним diagnostic
ticket от submit через bounded polling и сборку UTF-8 подписей до попытки cleanup
на каждом выходе. Capabilities/results локальны вызову, shared mutable globals
нет. Строки принадлежат caller и доступны после retirement target.

Сборка/запуск: [пример resident-debug](../examples/resident-debug/README.ru.md).
Native lifetime, statuses, смысл next PC и ограничения:
[owner location](../runtime/OWNER-LOCATION.ru.md).

Структура: reusable readers здесь, интерфейсы в module/, native capture/projections
в runtime/diagnostics/, owner publication в resident_threads/, приложения-примеры
в examples/. Каждый handwritten source — до500 строк, с описанием ownership и
threading на английском и русском.

Residentstack: ограниченные цепочки вызовов своего owner, [RU](../runtime/OWNER-STACK.ru.md) /
[EN](../runtime/OWNER-STACK.md), с [рабочим примером](../examples/resident-stack/README.ru.md).

Memoryview: согласованная статистика native арены, [контракт](../runtime/MEMORY-STATISTICS.ru.md),
[пример](../examples/memory-statistics/README.ru.md).
