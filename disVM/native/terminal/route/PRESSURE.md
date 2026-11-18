# Native backlog diagnostics / Native-диагностика накопления

Each Unibus route automatically observes its general, UI, service and typed
recipient queues. Crossing either enabled upper threshold produces a local
native HIGH notice. Recovery requires both enabled dimensions to return to or
below their lower thresholds. The defaults are 1024 unread records or 8 MiB of
copied payload; lower thresholds are half those values. These are observation
thresholds, not memory quotas or admission limits.

The broker continues receiving S-records and clients keep their own independent
blocking receives. A warning never suspends a guest, changes priority, balances
execution, closes a client or concludes that its interpreter/JIT has hung.

RU: Накопление очереди автоматически создаёт native-уведомление. Брокер не ждёт
его чтения и не вмешивается в выполнение клиента. HIGH означает превышение
наблюдаемого порога; отладчик самостоятельно определяет зависание исполнения.

## Native API / Native API

[`pressure.h`](pressure.h) defines local C values, not a new S-record schema.
The host pins the route through each call and retires all consumers before free.

| API | Result |
| --- | --- |
| `styxbidirroutepressurelimits` | Atomically copy valid local high/low policy and reevaluate current queues |
| `styxbidirroutepressurewait` | Block only the native diagnostics consumer until a notice, retirement or timeout |
| `styxbidirroutepressurenext` | Copy one pending notice; RECORD/EMPTY/END, no application data consumption |
| `styxbidirroutequeuemetrics` | Coherent counts/peaks, lifetime admission/delivery totals and oldest-record age |

```c
StyxRoutePressureLimits limits = {1024, 512, 8 * 1024 * 1024, 4 * 1024 * 1024};
/* Check each result; one native diagnostics consumer owns this inbox. */
if (styxbidirroutepressurelimits(route, &limits) &&
    styxbidirroutepressurewait(route, 5000))
{
    StyxRoutePressureEvent event;
    if (styxbidirroutepressurenext(route, &event) == STYX_CONTROL_RECORD)
    {
        /* Report locally using an agreed host interface; never serialize native struct layout. */
    }
}
```

Setting an upper threshold to zero disables that dimension; its lower must also
be zero. Both zero disables observation. Enabled lower thresholds must be strictly
below their upper thresholds. Invalid input leaves policy and pending notices
unchanged. Reapplying identical limits is idempotent. Explicit reconfiguration
is marked CONFIG when it affects a pending or active warning. Disabling a high
warning is a configuration change, not a false LOW recovery.

## Coalescing and lifetime / Объединение и владение

There is one preallocated diagnostic slot per possible recipient: three ordinary
inboxes plus up to eight typed recipients. No diagnostic allocation, callback or
wire operation occurs on enqueue/pop. A slow diagnostics consumer cannot create
another growing queue. Ordinary application messages remain lossless dynamic
FIFOs; diagnostic notices are coalesced state observations.

Repeated arrivals while a queue remains high do not emit repeated events.
Unread HIGH/LOW/CONFIG changes for one queue merge into its pending slot. The
event carries the union of transition kinds, latest high state, latest local
generation and a saturating number of coalesced transitions. `next` selects the
oldest pending queue and refreshes its metrics at delivery. Thus a HIGH followed
by LOW before reading is one event with both bits, latest high=false and the
current snapshot; it is not two stale application messages.

Generations are route-local diagnostic ordering, not remote provider generations
or access rights. Pending notices drain after retirement, then END. Draining
application data after retirement cannot resurrect diagnostic notices after END.
All storage is owned by the route and disposed after its ordinary joined lifetime.

## Metrics / Метрики

| Field | Meaning |
| --- | --- |
| `queue` | Existing current/peak unread records and payload bytes |
| `received`, `received_bytes` | Lifetime committed admissions |
| `delivered`, `delivered_bytes` | Lifetime successful caller copies; SMALL does not count |
| `oldest_milliseconds`, `age_valid` | Age of the oldest committed record from native monotonic admission time |
| `saturated` | At least one lifetime diagnostic counter reached its representable maximum |

Empty payloads count as records. Byte counts exclude allocator metadata; UI uses
the copied native decoded message extent. Totals saturate explicitly rather than
causing channel failure. Exact unread occupancy retains its original checked
accounting. A failed admission timestamp, failed clock query or regressing clock
only makes age unavailable; it never refuses data or changes access rights.
Age follows the current FIFO head and becomes unavailable when the queue empties.
No timer worker, recipient polling or instantaneous-rate estimator is introduced.
Native hosts can calculate rates from successive totals and their own interval.

## System messages and remote forwarding / Системные сообщения и удалённая сторона

These are automatic **local native** diagnostics. The existing `/dev/sysevents`
contains VFS/namespace event types; queue warnings are not disguised as filesystem
changes. This change does not add a `sysmessages` device or send a new tag-zero
command to the remote endpoint.

Both endpoints can run the same observer. Delivering the local warning to both
sides still needs an agreed tag-zero service encoding with connection/side and
recipient identity. No numeric tag, command ID, side identity, wire serializer or
permission policy is invented here. A native channel-service adapter can consume
one notice and project it through that future agreed interface. No native struct
or pointer may be copied to the wire.

RU: Автоматические локальные notices уже есть. Показ в `sysmessages` и пересылка
на оба конца требуют согласованного служебного формата тега 0. Существующий
VFS-журнал не подменяется фиктивными событиями; новые wire-команды не назначаются.

## Verification / Проверка

Six [original-Styx scenarios](../tests/pressure/cases.cmake) exercise count-only
and byte-only thresholds on 512 unread general/UI/typed messages. A separate
native diagnostics consumer blocks before publication and wakes while recipient
data remain unread. The fixtures verify one HIGH per queue, no flood, automatic
LOW on drain, coherent age and exact received/delivered totals.

The [deterministic native unit](../tests/pressure/unit.c) tests oldest-head age,
empty records, SMALL, failed/regressing clocks, failed timestamp capture, default
thresholds, mixed-dimension hysteresis, coalesced HIGH/LOW, configuration/disable,
idempotence, oldest-pending ordering, post-stop draining and saturated totals.
These diagnostic-clock tests do not claim physical carrier timing or remote
notification delivery. Existing Unibus tests continue covering real connection
retirement, cancellation, allocation failure and independent application queues.
