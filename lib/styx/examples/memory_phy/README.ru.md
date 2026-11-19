# 9P через custom memory PHY

Copyright (c) 2026 bhgv

[English](README.md) · [Русский](README.ru.md) · [Examples](../README.ru.md) · [Library](../../README.ru.md)

## Что и зачем

Настоящий native 9P-клиент и исходный libstyx server обмениваются wire-сообщениями через ограниченные буферы памяти. Носитель не создаёт socket, pipe, eventfd или файловых дескрипторов. Консольный stdout используется только для PASS/диагностики процесса. VM, GUI и Unibus для этого примера не нужны.

Это пример подключения своего драйвера через PhyOps, а не новый 9P codec. Файловые операции остаются обычными: Tversion, Tattach, Twalk, Topen, Tstat, Tread, Tclunk. Физически оба направления доступны, но это обычная client/server сессия; PHY_BIDIR_SESSION и скрытый reverse-server не включаются.

## Сборка и запуск

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/styx-memory-phy
ctest --test-dir build -R '^styx-memory' --output-on-failure
```

Из корня пакета. Аргументов командной строки нет. Ожидается PASS real 9P over memory PHY; код 0 — RPC и окончательное освобождение обоих endpoints проверены. Код 1/диагностика stderr — ошибка setup/RPC/lifetime. Ошибка native join завершает процесс через _Exit, без освобождения памяти потенциально живого worker.

Пример Linux-only: pthread, GCC/Clang atomics и binary futex wait/wake. Это зависимости учебного драйвера, не PhyOps ABI или portable codec. Минимальный STYX_HOSTED=OFF не собирает этот пример и остаётся без таких зависимостей.

## Состав и настройки

| Файл/параметр | Значение и назначение |
| --- | --- |
| memory.c | Factory, adoption, shared lifetime |
| io.c | Byte prefixes, circular buffers, backpressure |
| wait.c | Native wait/wake без fd; один monotonic deadline на wait |
| server.c / main.c | Исходный libstyx server и реальные client RPC |
| MEMORY_CAPACITY | 16 входных байтов на направление; не размер 9P frame |
| max_read / max_write | 3/2 байта на callback; границы фрагментов не границы сообщений |
| cut_after | 0 в нормальном примере; тестовое значение 5 моделирует подтверждённый prefix с ERROR/UNCERTAIN |
| MemoryStats | Нулевой caller-owned объект, живущий до destroy обоих endpoints |
| msize / timeout / limit | 256 / 3000 мс / 4 для native клиента; не глобальные значения протокола |

## Владение и последовательность

1. [memory_pair](memory.c#L60) создаёт два owned RemotePhy. [phycreate](../../docs/reference/phycreate.ru.md) копирует ops/info и принимает state только при успехе; factory освобождает partial construction самостоятельно.
2. Server thread принимает один endpoint через [styxphyinit](../../docs/reference/styxphyinit.ru.md); main передаёт другой в [styxclientmountphy](../../docs/reference/styxclientmountphy.ru.md). Оба constructor поглощают grant при любом исходе.
3. [memory_server](server.c#L45) владеет server/ops/file table до styxend. Main ждёт фактического startup через native condition, не обращаясь к активному server объекту.
4. [use_file](main.c#L20) получает отказ write-open на readonly status, затем отдельным fid выполняет read/stat, EOF на signed64 max offset и чтение с offset 7. Rerror открытия не разрушает исправное соединение.
5. [styxclientshutdown](../../docs/reference/styxclientshutdown.ru.md) и [styxclientfree](../../docs/reference/styxclientfree.ru.md) завершают client owner. Peer наблюдает disconnect, server заканчивает работу; main делает join до освобождения startup state.
6. [memory_destroy](memory.c#L41) освобождает shared allocation только после второго endpoint. Wake callers обязаны закончить до final free; buffered prefix может пережить уничтожение другого endpoint.

## Контракт драйвера

Read/write используют trylock и не ждут peer consumption. Пусто/полно/контеншн дают AGAIN с нулевым count, не EOF. Положительный count — принятый prefix. При cut_after возвращаются count и ERROR/UNCERTAIN вместе: prefix сохраняется, повторять весь запрос нельзя. После peer disconnect сначала дочитываются принятые байты, затем выдаётся EOF.

Wake — sticky local work, не wire-сообщение. wait сначала сбрасывает binary latch, потом проверяет persistent predicates и wake flag; kernel отказывается спать, если notifier уже изменил latch. Здесь нет 32-битного номера очереди или счётчика поколений: слово futex принимает только 0/1. Timeout 0 — poll, -1 — infinite wait; положительный timeout использует один CLOCK_MONOTONIC deadline на все EINTR/повторы. Это не обещание realtime scheduler latency.

PHY_STREAM/ORDERED/RELIABLE объявлены потому, что memory carrier сохраняет byte order и сообщает отказ. PHY_PACKET не подходит напрямую 9P. Настоящим SPI/I2C/CAN/радиоканалам могут требоваться reassembly, упорядочивание, контроль ошибок/потерь и half-duplex arbitration ниже этого интерфейса. Не выставляйте флаги вместо реализации этих свойств. connected в PhyInfo — local lifecycle, не heartbeat, разрешение доступа или доказательство состояния peer.

## Проверки и границы

styx-memory-construction внедряет ошибки allocation/mutex и первого/второго adoption, проверяя final frees. styx-memory-contract проверяет bounded prefixes, full/AGAIN, drain/EOF, uncertainty, timeout и 500 concurrent wake rounds без sleep. styx-memory-no-descriptor запускает тот же client/server с link-time отказом socket/connect/open/close/pipe/eventfd/read/poll путей проекта. Это guard linked code, не трассировка всех внутренних kernel calls libc.

Пример не реализует authentication, VFS union, реальный DMA/IRQ, потери пакетов, драйвер платы или remote-module migration. Вместо этого он оставляет заменяемую границу: замените PhyOps/context и способ создания endpoint, сохранив клиент/server и wire-протокол. Не переносите Linux futex-код на MCU без собственного native wait/wake adapter.

[Driver interface](../../remote_phy/phy.h) · [Source factory](memory.c) · [SDK integration](../../docs/embedding.ru.md) · [Main-project carrier guide](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
