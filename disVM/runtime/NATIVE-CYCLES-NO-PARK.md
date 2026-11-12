# Dedicated guests never receive tracing park requests

`--native-spawn` now selects quiescent-only retained Cycles even when the caller
explicitly supplies `--gc-worker` or `--gc-event-cycles`. Previously those explicit
profiles bypassed the implicit native default and could request a world stop.
Fast and shared Cycles/Duke retain their existing concurrent algorithms; this
change does not replace root tracing with an unsafe concurrent `rungc` call.

| Execution profile | Retained Cycles admission |
| --- | --- |
| Implicit dedicated console profile | Already quiescent heap; event requests coalesced |
| `--native-spawn --gc-worker` | Already quiescent heap; explicit requests retained |
| `--native-spawn --gc-worker --gc-event-cycles` | Already quiescent heap; event requests coalesced |
| `--native-spawn --gc-no-cycles` | Cycles disabled; no implicit idle option conflict |
| Explicit single-thread reference with `--gc-worker` | Existing stopped tracing diagnostic |

The idle policy also completes the bounded color rotations after a request,
without depending on a second guest scheduling turn. Shared candidate requests
still execute before the retained route. A busy heap causes retained tracing to
retry; it must neither request parking nor pretend to have traced active roots.
Quiescent tracing still temporarily excludes new heap admission. This is not yet
removal of every global exclusion point or a complete concurrent tracing GC.
Unknown finalizers and unsupported graphs retain their conservative path.

## Joined resident accounting

Removing heap exclusion also exposed an older race in `yield_empty`: two native
residents could join different completed slots and concurrently add their counts
to the same `service.retired_*` totals. The original TSan run reported races on
`reaped`, `retired_turns`, `retired_releases` and `retired_reentries`.

All four additions now use the existing `service.sync`. The joined slot belongs
to exactly one reaper, and its own counters are read only after registrar join.
Neither join, wait, diagnostics nor slot destruction runs under this metadata
lock. No execution lock, register reload, thread pool or Prog migration is added.
`NATIVEWORKERS_REAP` now reports releases/reentries as well as turns. The shared
resident evidence checker reconciles both early-reaped and shutdown-joined slots
against the final production totals, requiring each permanent slot ID once.

`managed_workers.c` composes private state, rootless wait/reap, thread execution
and registrar startup fragments in `resident_threads/`. They remain one native
translation unit with unchanged private linkage. Distribute that directory with
the runtime; CMake compiler dependency tracking rebuilds the coordinator after
an include changes. No generated source or new build dependency is required.

## Child retirement publication

Repeated TSan request stress also caught parent `killwait` reading `child->state`
under `channel_queue_mutex` while the child wrote `Pexiting` outside that guard.
The child now publishes `Pexiting` under the same queue/registry mutex immediately
before removing its process root. Sealing, final dispatch accounting and all
stack/module destruction retain their ordering; destruction stays outside the
queue mutex. The parent still sends requests to the resident owner rather than
executing the child's cleanup itself.

## Regression scope

`disvm_gc_requests` runs five GC profiles with C/JIT and both polling paths:
20 real VMs, 160 permanently assigned residents and 640,000 actual GC requests.
Each request checks synchronized world state while its owner is admitted;
`stopping` and `stopped` must both remain false. Final tracing snapshots must be
zero, real native roots remain published during drain, and all RC/queues/threads
must retire. For event profiles the fixture first requests real quiescent Cycles
and waits for three completed color rotations, then checks the exact remaining
clean-request allowance and bounded fallback across all eight admitted owners.
The general allocation application cannot require a positive skip count: with
no quiescent window during allocation, its three epochs may finish only at exit. Explicit profiles failed the old fixture's idle-policy precondition.
The runtime GC application checks still require live roots, reclamation through
retained or shared collection, completed epochs and managed-error cleanup; they
no longer require the stopped collector itself to perform the reclamation in a
dedicated profile.

TSan covers native C/host/fixture code, not machine instructions emitted by JIT.
Acceptance evidence is Linux x86-64; this change does not certify ARM64/RV64 JIT.

# Закреплённые гости без парковки ради трассировки

Теперь `--native-spawn` выбирает резервную трассировку только в уже наступившем
покое, включая явные `--gc-worker` и `--gc-event-cycles`. Отключение Cycles не
получает конфликтующую настройку idle. Однопоточный диагностический эталон
сохраняет остановочную трассировку. Общие очереди Fast/Cycles/Duke продолжают
работать конкурентно; `rungc` не запускается поверх изменяемых активных корней.

Исправлена настоящая гонка учёта присоединённых потоков: четыре общих счётчика
обновляются под существующим мьютексом метаданных. Join, ожидания, вывод и удаление
слотов выполняются вне него. Запись `Pexiting` при завершении ребёнка теперь
использует тот же шлюз очереди/реестра, что и чтение родительским `killwait`. Проверка сопоставляет итоговые суммы с каждым
фактическим join, включая раннюю сборку слотов и остановку сервиса. Файл на 823
строки разделён по обязанностям с сохранением одной единицы трансляции.

Это ещё не полностью конкурентная трассировка: сборка в покое временно запрещает
новый допуск; неизвестные финализаторы и неподдержанные графы требуют дальнейшей
работы. Машинный код JIT не инструментируется C-TSan, ARM64/RV64 здесь не проверены.
