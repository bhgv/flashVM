# Shorter native metadata critical sections

This change addresses two repeated scans that all native mutators and collectors
could encounter under metadata mutexes. It does not remove payload guards or
permit tracing active register sets.

## Root identity distribution

The process-wide root registry used 32 buckets indexed by address bits above the
64-byte boundary. Objects separated by 2,048 bytes all entered one chain. Root
counts, ordinary removal and batch unlinking then scanned that chain while other
threads waited for the same metadata mutex.

The registry now uses 4,096 static buckets and mixes all native pointer bits.
On a 64-bit host its bucket heads occupy 32 KiB instead of 256 bytes. The table
still has no object quota and adds no native allocation/refusal path. Bucket
heads remain stable for process lifetime; existing ordinary, batch and staged
pointer roots continue to use the same private bridge while holding its mutex.
Occurrences remain a multiset; visitor order is not an API guarantee.

The distribution regression registers 8,192 real native identities at a 2,048-byte
stride, then publishes two batch occurrences per identity. It observes actual
bucket identities rather than recomputing the hash. It requires broad distribution
and short chains for this workload, checks all 24,576 occurrences, removes ordinary
roots without touching batches, clears batches and verifies the registry is empty.
Both atomic and forced-locked publication backends run. Private restoration of
the shifted 32-bucket index must fail with the exact distribution diagnostic.
This is not a cryptographic hash or a worst-case constant-time guarantee.

## Arena search cursor

Every allocation previously restarted a first-fit walk from the arena head.
Keeping thousands of blocks live caused repeated prefix scans under the pool
mutex. A private per-pool cursor now starts the search near the last allocation
or free. Allocation searches the suffix, then wraps through the prefix once.
Each interval validates block geometry. Splitting moves no existing headers;
free makes its validated block the next search start. A full last block wraps
the cursor to the first header. Quiescent destruction clears the cursor together
with the arena and mutex.

The cursor and all block metadata use the existing pool mutex. Allocation/refusal,
generation assignment, invalid-pointer contracts and callback-after-unlock rules
remain in force. There is no coalescing, block relocation, compaction or managed
payload destruction under that mutex. Raw GC walks still require independent
quiescence; their block addresses remain stable. Pool is a private native owner
whose consumers must be rebuilt together. Bhdr, Heap, Type and Dis64 layouts do
not change.

The search regression keeps 4,096 actual native blocks live and observes the real
number of inspected headers in a private diagnostic build. Initial allocations
visit 4,096 headers, rather than repeatedly scanning the growing live prefix.
It then exhausts a real arena, reuses an address with a fresh generation, creates
two holes, consumes the later hole, wraps to the earlier one, checks payloads and
destroys the empty pool. Restoring head-first search must fail its deterministic
work allowance; elapsed time is never the oracle. Existing eight-writer allocator
tests retain 16,000 allocation/write/size/free rounds and corruption/reentrant-hook
controls. The diagnostic observer is not present in the production allocator.

## Test resource accounting

The three full GC application suites and the large-owner matrix declare four
CPU slots to CTest. With
`ctest -j4` they do not compete with other test processes. Their 180/180/130-second
deadlines and every per-VM assertion remain unchanged. This scheduling property
does not serialize the native threads inside a VM: dedicated guests, Fast, Cycles
and Duke retain their real OS threads and existing admission rules.

The replacement test observer now captures the actual old-key bucket heads
before withdrawal. It no longer tries to enumerate the registry by fabricating
keys from the previous 32-bucket address formula. All twelve exact-stage negative
controls still require the same diagnostics before malformed metadata can be
relinked; no production replacement algorithm changes.

Native registry, batch, replacement and pool suites also pass under TSan;
replacement retains all twelve exact-stage controls. Distribution and pool-search
controls run under TSan as well. The standalone positive suites pass UBSan.
An additional 316 actual VM TSan runs cover GC request bursts, resident root
observers, module cycles, Duke hard-limit progress, packet fairness and the
156-profile large-owner matrix. JIT-emitted instructions are not instrumented.

The selected acceptance matrix contains 102 checks. The three previous aggregate
GC timeouts pass within their original deadlines (approximately 129, 29 and 43
seconds). After correcting that observer and scheduling the large-owner matrix
with its CPU reservation, both targeted revalidations pass as well. This is a
full set of passing checks after targeted revalidation, not a claim that the
first parallel run had no failures.

On this host a full native/JIT application profile previously took about 38–42
seconds, approximately 27 seconds after root distribution alone and 15 seconds
after the arena cursor. These are observed timings, not benchmark guarantees;
GC scheduling changes pass counts. Functional roots, reclamation, thread identity
and zero-migration evidence remain mandatory. Execution evidence is Linux x86-64;
TSan instruments native C/host/fixture code, not emitted JIT instructions.

# Сокращение проходов под замками метаданных

Реестр корней больше не складывает адреса с шагом 2 048 байт в одну цепочку:
4 096 статических корзин и смешивание адреса сокращают поиск под общим мьютексом.
Обычные, пакетные и временные корни сохраняют прежнее владение и число вхождений.
Нового отказа выделения памяти для индекса нет.

Аллокатор начинает поиск от защищённого курсора, при необходимости один раз
обходит префикс. Удаление указывает на повторно доступный блок; поколения адресов
не переиспользуются. Заголовки не перемещаются, блоки не объединяются, callbacks
остаются вне мьютекса. Сырой обход GC по-прежнему требует отдельного покоя.

Проверки используют реальные native-объекты, обычные/пакетные корни и реальную
арену. Измеряется число просмотренных заголовков, а не время; возвращение старых
поисков обязано давать точные ошибки контролей. Три тяжёлых GC-теста и матрица широких объектов получают
CPU-слоты CTest без увеличения таймаутов и без сериализации потоков внутри VM.
Общая резервная трассировка всё ещё не стала полностью конкурентной.
