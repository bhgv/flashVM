# Negotiated ordered broker / Брокер с согласованием порядка

`styxbidirordered` is an explicit alternative broker for an **owned, fresh**
`StyxBidirControl`. Once the native host calls `styxorderedoffer`, the broker
handles OFFER/ACCEPT/REJECT, packet encoding, RX validation, per-tag queues,
renewal requests and matching ACKs on its dedicated native workers. Application
threads use fixed-tag inbox capabilities; they do not drive the transport or
interpret its service records. Existing flat/scoped brokers keep their profiles.

Расширение: **`unibus.order.v1`**. После native-запроса предложение и ответ
обрабатываются брокерами автоматически. Приложению выдаётся только handle его
фиксированного внутреннего тега. Служебный обмен не нужно обслуживать из Limbo.
Создание этого брокера выбирается явно; существующие запущенные брокеры не меняются.

## Framing and namespace / Обрамление и пространство имён

Outer traffic remains **ordinary S-records** throughout the connection:

| Outer tag | Payload |
|---|---|
| 0 | Native named services; unknown bodies go to a separate native inbox |
| Negotiated nonzero lane | One [CRC-5 sequence prefix](../README.md) and one complete inner S-record |

The chosen lane tag is directional and negotiated, never a global constant.
The inner S-record carries the application tag and opaque payload. Its own tag0
is application-local. A client cannot read/write the outer root0, select another
client's tag, change offers, or call native renewal controls through its handle.
The constructor predeclares unique authorized inner tags before starting workers.
Incoming unknown inner tags or data on an unnegotiated/wrong outer lane retire
this explicit broker. Negotiation never grants filesystem or ambient namespace
access: the underlying control retains the original captured Styx capability.

This is an Unibus payload extension, **not a modification of Styx/9P messages**.
Original Tread/Twrite/walk/fid handling remains in libstyx. A compatible peer must
select this same envelope profile. FlashVM receives the same portable service
codec. The [ordered terminal adapter](../../bridge/ORDERED.md) now connects this
broker to existing real local value channels/AS wrappers through a captured launch
descriptor. Native embedding explicitly selects it; existing desktop/editor CLI
paths are not silently changed into remote launchers.

Внешнее обрамление не переключается. В root0 идут службы, а в согласованном
ненулевом теге — пакет с номером и внутренней S-записью. Поэтому внутренний
клиентский tag0 не может превратиться в системную команду. Grants задаёт native
host заранее; сетевой peer не создаёт новые клиентские полномочия.

## Named service v1 / Именованная служба v1

The **outer root0 body** contains exactly two complete nested S-records:

1. Inner tag0, exact ASCII identity `unibus.order.v1`, no NUL terminator.
2. Service-local command tag from the table below. Its body contains exactly
   six **original canonical unsigned S-integers**, in this order:
   `operation, lane, limit, window, tag, boundary`.

| Local command tag | Meaning | Active fields |
|---|---|---|
| 1 | OFFER this sender's TX lane | Nonzero operation/lane, body limit, reorder window |
| 2 | ACCEPT exact proposal with bounded limits | Same operation/lane; limit/window no larger than offer |
| 3 | REJECT | Echo all offered fields; no activation |
| 4 | RENEW one inner TX counter | Nonzero operation/lane, inner tag including0, final old boundary |
| 5 | ACK renewal | Echo all renewal fields |
| 6 | NACK renewal | Echo all renewal fields, preserve old counter |

Offers require zero `tag/boundary`; renewals require zero `limit/window`.
Operation and boundary are unsigned64, lane/tag are unsigned32. Limit counts the
**encoded inner tag plus payload**. Window is1..2^63-1; all integers must be
canonical and unsigned. No serialized native struct, trailing record/integer,
negative number, zero operation or zero lane is accepted for a known service.
No globally reserved service-version integer was allocated: the nested identity
separates these command tags from every other root service.

Codec: [sordctl.h](../../../../../flashVM/include/flashvm/sordctl.h), C89 ABI,
two explicit uint32 parts for64-bit values. Other root services remain opaque;
an unparseable or different identity returns UNKNOWN. Once the exact checked
identity matches, malformed command content is a protocol error. Independent
Python integer/zlib vectors verify the complete encoding, not just round trips.

Имя отделяет локальные номера команд от других служб root0. Поля идут в указанном
порядке и кодируются исходными S-целыми. Это относится к редким служебным сообщениям;
в каждом пакете данных остаётся лишь компактный префикс номера, **без поколения**.

## Directional activation / Включение направлений

Native host selects each TX lane and initiates a strictly increasing nonzero
operation. RX and TX negotiations are independent. Receiver selects the smaller
local/offered limit/window and allocates its reply **before** activating RX and
queueing ACCEPT under the same broker lock. Sender activates TX only on the
matching bounded ACCEPT. Wrong matching lane/limits, conflicting repeated result,
or malformed known service retire the broker; stale unrelated IDs do not unfreeze
anything. Exact most-recent duplicates are idempotent. A rejected TX offer permits
a later higher operation; timeout leaves its pending state unchanged.

`styxorderedofferwait(peer, operation, timeout)` waits for resolution of that
exact latest operation. Return1 means ACCEPT **or** REJECT; inspect TX readiness.
An active v1 lane is immutable until a new session; another offer receives REJECT.
This is not yet dynamic lane remapping or an automatic multi-carrier scheduler.

## Queues and renewal / Очереди и сброс

`styxorderedservicewait/servicenext` provide a separate blocking native root-service
inbox for unknown services, one copied record at a time. Applications cannot wait
on or consume this inbox through their handles.

One native RX dispatcher handles root services and feeds independent per-inner-tag
sequence queues. One native TX worker sends a dynamic FIFO of retained encoded
packets and services into the ordinary control. Only the lower transport worker
waits for physical data; only a selected application's native wait waits for its
own queue. No guest callbacks, forced suspension or cooperative guest servicing.

Sending allocates the packet spool **before** assigning a number; refusal for
capacity/negotiated bounds does not consume an ordinal. TX retains the same bytes
through actual short-write acknowledgements. The FIFO places a renewal after
all previously accepted old packets. The selected tag refuses new sends while
pending; other tags and the opposite direction continue normally.

On encoding MAX, a renewal is queued automatically. Native `styxorderedrenew`
can initiate an earlier renewal. The receiver reserves its ACK before changing
state and waits until the final old selected-tag record has actually been copied
by `styxorderedclientnext`; SMALL is not consumption. That copy queues the ready
ACK without waiting for I/O. Repeated pending requests cannot ACK early. A completed
duplicate request replays its ACK without resetting new data again. Matching TX
ACK resets only its tag to zero; NACK preserves the old next number/exhaustion.
A failed allocation of an automatic MAX renewal retires the broker with NOMEM;
there is no silent wrap or success claim for unsent data.

The old-delivery barrier here relies on **one reliable ordered Styx stream**:
old data precedes its root0 renewal, and new selected-tag data waits for ACK.
There is no extra generation in each data packet. Additional physical lanes must
supply their own old-delivery retirement barrier before reusing zero. Automatic
striping, lane growth, remote retry/reconnect reconciliation and launcher selection
are not implemented in this broker.

Сбрасывается лишь счётчик конкретного внутреннего тега и направления. Запрос
стоит после старых пакетов в FIFO, ACK ждёт выдачи последней старой записи.
Остальные теги продолжают работу; Limbo не обслуживает брокер и не приостанавливается
им принудительно. При разрыве уже готовые записи доступны для поштучного чтения.

## Native use / Применение

```c
unsigned long tags[] = {0, 7, 9};
/* control is newly owned; this constructor consumes it even on failure. */
StyxOrdered *peer = styxbidirordered(control, 4096, 1024, tags, 3);
/* Native host checks peer, then proposes its TX direction. */
StyxOrderedClient *client = styxorderedclient(peer, 7);
int queued = styxorderedoffer(peer, 1, chosen_tx_lane);
/* The peer independently proposes its own TX lane for this side's RX. */
int ready = styxorderedwait(peer, STYX_ORDER_TX, 3000);
/* Applications use only client send/wait/next; native host retains peer. */
```

Keep the owner alive through all client calls. Stop is nonjoining; final free
requires all external callers to retire, then joins workers and consumes control
and client capabilities. Never free from a broker worker.

## Acceptance / Проверки

Original libstyx server/client over actual explicit bidirectional native PHY pumps:
three-byte reads, deferred idle Tread, two-byte Twrite acknowledgements; independent
RX/TX offers, REJECT then retry, duplicate controls, malformed ACK, premature data,
inner tag0 isolation, remote body limits, concurrent bidirectional renewal,
unrelated tag progress, automatic MAX renewal and actual carrier loss.

A workerless coordinator fixture injects allocation failures before mutation,
checks stale/wrong confirmations, NACK retaining the old next ordinal, and duplicate
ACK/request handling after new data already exists. A separate Python unsigned
integer/zlib oracle checks1212 complete service bodies. Public headers link from
C++98 and codec acceptance endpoints compile as C90. These are native engine tests,
not acceptance of an automatically upgraded FlashVM launcher or full multi-lane I/O.

## Blocking send and lifecycle subscribers / Ожидающая отправка и подписки

[Per-client sendwait and sticky native retirement](../../bridge/ORDERED.md) add
precise timeout/size/memory/refusal results without changing the original send1/0
ABI. Lower-control notification now wakes clients/subscribers without waiting
for dispatcher progress. No guest callback, global client drain or forced guest
pause is introduced. Source-pinning subscriptions must fence owner lifetime.

## Orderly EOF / Корректный EOF

Clean lower-control EOF is an ordered barrier: the RX dispatcher first drains
complete records already accepted by the lower queue, then closes the engine
and issues its sticky retirement notification. The lower retirement callback must
not close the engine ahead of those records. Physical faults, protocol failures
and explicit stop retain their independent immediate wake path.

`styxorderedagreed(peer, direction)` reports historical completed agreement,
including after retirement. It is **not** liveness, send permission or a restart
operation. The [explicit native CLI](../../cli/README.md) uses it only with clean
EOF so that startup observation delayed until after orderly retirement can still
drain accepted client records. `styxorderedready` continues to require a live peer.

Чистый EOF не обгоняет уже принятые записи. Ошибки и явный stop по-прежнему
пробуждают подписчиков независимо от продвижения dispatcher. Исторический факт
согласования не даёт права отправлять после завершения соединения.

## Captured construction / Создание по закреплённым каналам

The [shared native attachment](ATTACHMENT.md) validates a display-independent
profile before opening captured Styx channels, creates this broker and queues its
initial offer. A separate entry borrows an actual pre-adoption launch handoff.
The CLI and optional FlashVM bridge use this same path; the caller keeps its
original session, namespace and handoff ownership.

## Operation-local cancellation / Отмена отдельной операции

Cancellable variants now wait for only the selected client. Native subscriptions
wake the broker condition without stopping it; the dispatcher publishes readiness
under the same mutex. A combined wait/copy prevents another open from stealing a
checked head. Cancellation and subscription failure consume no message/ordinal
before admission. The [ordinary record-file provider](../files/README.md) forwards
the current native FD cancellation scope into these waits; no guest polling loop
or managed callback is added.
