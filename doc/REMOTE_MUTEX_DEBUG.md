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
