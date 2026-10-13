# Ожидания сетевых мутексов / Remote mutex waits — v0.71

Блокирующий `set` remote proxy регистрирует ожидание в локальном mutex registry
на всё время попыток RPC. Запись содержит context ID, source/function/line/offset,
полученные при входе в операцию. Ожидание удерживает native mutex reference;
запись снимается при успехе, отмене worker, закрытии или ошибке до освобождения
этой ссылки. Nonblocking contention не оставляет записи после возвращения.

`fvm_debug_mutex_snapshot` теперь показывает такие ожидания. Новый
`fvm_debug_mutex_detail_snapshot` дополнительно возвращает признак remote и
скопированный capability URI (512 bytes). Оба API копируют coherent registry
snapshot под lock без RPC; размер существующего FVM_DebugMutex не изменён.
При нехватке capacity возвращается NOT_FOUND и требуемое число строк; буфер
строк не изменяется. Запрос count с capacity=0 разрешён. Повторный вызов требует
учитывать изменение числа потоков между запросами.

`owner/state/deadlocked` отражают только наблюдаемые локально ownership edges.
Remote proxy не знает владельца, захватившего authority через другой процесс или
другой proxy. Нулевой owner не доказывает, что authority свободна. `deadlocked`
проверяет локальный граф; отсутствие цикла не исключает распределённый дедлок.
Новая диагностика не выполняет разрешение URI aliases, получение remote owners
или слияние графов разных брокеров. Source maps должны предоставить инструменты
сборки/embedding; тест задаёт source/line вручную.

`mutex_remote_debug.c` использует настоящий loopback TCP broker и worker,
проверяет source/URI ожидания и сохранение буфера при малой capacity. Далее
проверяются resume после release, cancellation с RESOURCE_CANCELLED, closure с
RESOURCE_CLOSED и удаление wait rows. Также проверяется отсутствие native handles
после teardown. При OS_PORTS=off live TCP часть исключена.

## English

Blocking remote mutex acquisition now exposes a local wait row throughout its
RPC polling loop. The entry copies the requesting context/source/function/line/
offset and retains a native reference until success, cancellation, closure or
failure removes the entry. Nonblocking calls leave no persistent wait row.

The existing snapshot includes remote requests. The new endpoint-aware
`fvm_debug_mutex_detail_snapshot` also copies the remote flag and 512-byte URI.
Both snapshots hold only the local registry lock and perform no network requests.
The existing FVM_DebugMutex ABI is unchanged. Insufficient capacity returns
NOT_FOUND and required count without changing row buffers; capacity-zero count
queries are supported. Counts may change before the next snapshot call.

Ownership and cycle flags represent locally observed edges only. A proxy cannot
infer owners reached through another proxy/process; owner zero does not prove
that the authority is free, and no local cycle does not exclude a distributed
one. URI alias resolution, authoritative remote owners, cross-broker graph
merging and full distributed deadlock acceptance remain open. Source maps must
be supplied by embedding tools; the regression test manually sets its source
and line.

The live TCP regression covers wait locations/endpoints, atomic insufficient
capacity, release/resume, cancellation, closure, wait removal and final native
handle teardown. OS_PORTS-disabled profiles omit live TCP cases.

Проверки v0.71 / verification: headless 247/247; targeted mutex/GC/broker/
debugger checks 35/35 in AGG, Cairo, portable and ASan/UBSan. Portable excludes
live TCP. ASan covers heap/global memory, UBSan active; leak detection and stack
ASan disabled. Linux profiles only, no other platform runtime certification.

## v0.80 — Смешанный локальный цикл / Mixed local wait cycle

Новый `mutex_mixed_deadlock.c` передаёт одну TCP proxy через retained queue двум
OS workers. Один worker держит локальный mutex и ждёт proxy; второй держит эту
же proxy и ждёт локальный mutex. Debugger обнаруживает обе стороны цикла и
проверяет context IDs владельцев/ожидающих, строки захвата/ожидания и URI remote
строки. После cancellation проверяются отсутствие wait/owner/cycle записей,
освобождение authority и локального mutex, повторный цикл и полная очистка registry.
Исправление реализации не потребовалось. Отдельные proxy/process owners по-прежнему
не объединяются: тест не подтверждает распределённое обнаружение дедлоков.

The new regression shares one retained TCP proxy between two OS workers and
creates a cycle with a local mutex. Both cycle rows, requester/owner context IDs,
acquisition/wait locations and the remote URI are checked. Cancellation releases
both locks and removes waits; a second cycle verifies reuse, and teardown leaves
an empty registry. No implementation change was needed. Owners reached through
separate proxies/processes remain outside this locally observed graph.

Проверки / verification (Linux): default AGG with fonts/codecs/SQLite,
examples disabled: 242/242 tests; new mixed-cycle test: 100 consecutive passes,
two cycles per run. Headless OS_PORTS ASan/UBSan: 5/5 targeted tests
(mixed/local/remote mutex, nested network fault, broker adapter lifetime).
LeakSanitizer cannot enumerate sandbox `/proc` tasks and is disabled through
linked sanitizer defaults; stack use-after-scope instrumentation is disabled.
Portable NO_OS_PORTS: new test builds/passes with the live TCP case excluded.
These checks do not certify other platforms or full stages 6/7 acceptance.
