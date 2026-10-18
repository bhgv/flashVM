# Время жизни графов брокера v0.78 / Broker adapter lifetime

Ранее channel receive/peek использовали persistent peer context. При conversion
packet с shared native capabilities десериализовался в VM graph для экспорта URI,
но этот промежуточный graph/strings/native wrapper references оставались до stop.
Repeated inspection/receipt накапливали heap без видимых клиенту объектов.

Опции 6/9 теперь используют context на один dispatch. После формирования wire
reply (или любой ошибки) освобождаются все intermediate objects, native references,
string buffers и font/constructor caches. Broker pointer borrowed и перед ordinary
context destroy снимается, чтобы не уменьшать authority reference count.
Temporary replicas помечаются finalized: преобразование транспорта не должно
выполнять дополнительные user __gc в скрытой VM. Native resource cleanup остаётся
обычным; source и настоящий receiving VM сохраняют свои правила финализации.

Mutex set/reset сохраняют stable peer context identity: owned locks должны
принадлежать одному клиенту между RPC. Export registry канонических native handles
тоже сохраняется до stop; повторный экспорт одного handle не добавляет entries.
Новая модель не вводит leases или автоматическую очистку departed ownership peers;
не означает bounded total service RSS при неограниченном числе distinct clients/
exports. Request graph work всё ещё ограничивается существующими record budgets.

[fvm_broker_stats](../include/flashvm/capability.h#L34) возвращает coherent lock snapshot peer_contexts, exported_resources,
adapter_contexts и saturating adapter_requests. Это counts, не byte accounting;
запрос не читает меняющиеся VM heaps, не выделяет память и не выполняет RPC.
После stopped возвращается ERROR без изменения output; invalid arguments также
не меняют output. Как и другие broker APIs, caller должен удерживать broker handle.

broker_adapter_lifetime сначала воспроизводит persistent channel peer после peek,
затем проверяет 200 repeated cyclic/shared-mutex inspections и 200 receipts,
client GC, zero active adapters/zero wire-only peers, stable export count, creation
counter и сохранение mutex ownership peer после set/reset. Closed nested cause
regression проверяет zero active adapters также после failed peek/receive.
OS_PORTS off исключает live TCP. Общая приёмка GC/debugger остаётся открыта.

## English

Wire receive/inspection used to materialize intermediate capability-bearing graphs
in persistent peer contexts, retaining their heaps until broker shutdown. Opcodes
6/9 now use a scoped context for one dispatch. Graph/native wrapper/string/cache
storage is destroyed on successful conversion and every failure path. Borrowed
broker ownership is detached before ordinary context teardown; hidden wire replicas
skip user finalizers while normal native reference cleanup still runs. Sender and
actual receiver VM finalization contracts are unchanged.

Stable mutex ownership contexts remain for set/reset identity across RPCs, and
canonical exported native handles remain until stop. Repeated export reuses entries.
This is not an ownership-lease protocol or bounded total RSS across unlimited
clients/exports; those lifetimes remain separate acceptance work.

[fvm_broker_stats](../include/flashvm/capability.h#L34) copies synchronized peer/export/active-adapter counts and a
saturating adapter creation counter without allocation, RPC or concurrent VM-heap
inspection. Stopped/invalid queries preserve caller output; callers must hold a
live broker reference. Counters are not memory byte totals.

Regressions cover 200 cyclic/shared-capability peeks and 200 receipts, receiver GC,
zero live adapters/wire-only peers, canonical exports, stable mutex ownership and
creation counters, plus cleanup on nested closed-capability failures. Networking
cases are omitted without OS_PORTS; full GC/debugger acceptance remains open.

Проверки v0.78 / verification: full headless 262/262; targeted broker/
channel/resource/mutex/GC/serialization tests 73/73 in AGG, Cairo, portable
and ASan/UBSan. Live fonts in AGG/Cairo only, TCP omitted in portable.
Heap/global ASan and UBSan active; leak detection/stack ASan disabled.
Linux profiles only; full GC/debugger acceptance remains open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_broker_stats` | function | [include/flashvm/capability.h:34](../include/flashvm/capability.h#L34) |
<!-- source-links:end -->
