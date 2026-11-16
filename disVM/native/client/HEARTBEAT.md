# Connection heartbeat / Контроль отзывчивости соединения

Runtime0.148 adds optional native heartbeat to `styxclientmountfd` connections.
One socket owner runs the policy for every provider/fid sharing that connection.
It requires no guest execution, Limbo loop, scheduler polling or guest pause.
Configuration is a host decision on a retained native client capability.

## Standard Styx / Стандартный Styx

The probe is an original-codec nine-byte Tflush. Its tag is `2*limit+1` and its
oldtag is `2*limit+2`; neither belongs to ordinary transactions or their paired
flushes. It does not cancel a real operation, allocate a fid, open a file, stat
metadata or require resource access. The standard
[Inferno flush(5)](https://www.vitanuova.com/inferno/man/5/flush.html) specifies an
Rflush even when oldtag is not a pending transaction. Original C libstyx handles
this before fid lookup. Applying that standard exchange as heartbeat is our
native policy, not a new 9P message or an Inferno heartbeat extension.

## Native API / Native API

```c
/* Own an already connected/authenticated-by-host transport. */
StyxClient *client = styxclientmountfd(fd, 8192, user, tree, 5000, 32);
/* After 5s without a valid reply: 1s observation windows, threshold3. */
int enabled = styxclientkeepalive(client, 5000, 1000, 3);
/* Mount consumes one owned client reference; observation is automatic. */
DVM_Provider *provider = enabled ? styxmountprovider(client, FVM_ACCESS_MASK) : NULL;
/* On the disabled/error branch the native host still owns client. */
```

`styxclientkeepalive(client, idle_ms, reply_ms, misses)` returns1 on configuration,
0 for NULL, legacy callback transport, invalid settings or a terminal connection.
Both times are1..120000ms; misses is2..16. All three zeros disable future probes.
Invalid settings leave the previous policy intact. Default is disabled, preserving
existing transport traffic and timing. The native owner wakes immediately after
configuration; the caller never waits for a protocol response.

`styxclientobserve(client, receive, state, release)` returns an owned `StyxWatch*`
or NULL and adopts state only on success. It immediately delivers a current
`StyxLinkEvent` snapshot and then state transitions. The snapshot contains a
local monotonic generation and state. Registration replay can race native push;
recipients reject older/equal generations. Use `styxclientunwatch` for nonwaiting
detach and release recipient storage only in the release hook. At most256 total
terminal/persistent watcher nodes can coexist, including detached live handles.

`styxclientlink(client, &snapshot)` copies the latest native observation without
I/O. The existing `styxclientwatch` remains one-shot, receiving the first terminal
FAILED, CLOSED or TIMEOUT reason. Persistent observers also receive suspicion
and recovery. Neither interface grants resource permissions.

| Observation | Meaning |
|---|---|
| `STYX_LINK_READY` | Initial negotiated session, or a valid response after suspicion |
| `STYX_LINK_STALLED` | Probe could not finish sending within an observation window |
| `STYX_LINK_SUSPECT` | A completely sent probe missed a reply window |
| `STYX_LINK_TIMEOUT` | Configured silence threshold exhausted; connection retired |
| `STYX_LINK_FAILED` | Observed transport/framing/protocol failure |
| `STYX_LINK_CLOSED` | Explicit native shutdown |

A timeout is a policy decision about observed silence, not proof of remote death.
Network partition, CPU overload and a blocked server can produce the same symptom.
These states do not separately diagnose physical network failure or resource health.

## Scheduling and deadlines / Планирование и сроки

A valid complete response moves the next idle deadline. A legitimate pending
read does not suppress heartbeat. One independent probe uses fixed native
storage and no ordinary RPC quota. Existing transaction deadlines remain their
own policy; a responsive heartbeat does not extend a particular read/write.

A probe waits behind an already selected frame; cancellation flushes have priority
at frame boundaries. Once any probe bytes start, no other frame can interleave.
Send observation begins at probe admission, so local output blockage is reported
as STALLED rather than a missing server response. On complete send a fresh reply
phase begins. Partial send/receive progress never renews either phase's windows.

The first missed window emits suspicion; duplicate windows do not flood inboxes.
At the configured threshold the owner closes the connection and wakes pending
native callers with the existing precise-effect rules. Each phase has its own
captured budget. Any valid complete ordinary response also proves responsiveness:
it clears reply suspicion and refreshes its response window, but does not release
the probe's tags. The Rflush itself is still required to retire the probe.

There is only one outstanding probe. Misses count consecutive observation windows
for that probe, not duplicate Tflush requests or reused tags. TCP handles transport
retransmission. This avoids accumulating unresolved leases and ambiguous late
responses. An Rerror, malformed/premature response or duplicate inactive probe
response is a protocol failure. Standard9P has no wire generation to distinguish
a server's invalid duplicate after a tag has legitimately been reused.

Disabling/reconfiguring changes future probes only. An admitted probe retains its
original budgets and tags until Rflush or terminal retirement, including when its
first byte has not yet been sent. Normal final owner drop remains nonwaiting and
quiet; explicit shutdown remains distinct from heartbeat timeout. No automatic
reconnect, fid restoration or replay of uncertain effects is introduced.

## Provider events / События провайдера

Each mount automatically observes its client's health source. Initial READY at
generation1 matches the journal's initial state. Later suspicion, recovery and
failure invalidate CONNECTION|NETWORK; explicit shutdown invalidates CONNECTION.
The source generation advances only on changed observations and the journal
rejects delayed older replay. A late mount captures current state; a skipped
history conservatively invalidates ALL. Existing namespace watchers deliver one
event at a time to per-thread inboxes. No wire/inbox format was extended here.

A recipient uses the notification as a refresh hint and queries the retained
native client's current snapshot. Native callback contexts retain only journals,
not provider/guest pointers. Unmount never waits for an admitted callback.

## Acceptance and scope / Проверки и границы

The native suite has78 tests. Original C libstyx runs behind deterministic TCP
gates for repeated healthy probes, silent response loss without EOF, recovery by
Rflush and ordinary traffic, pending reads, disabled/reconfigured admitted probes
and invalid Rerror. Packet checks reject tag collisions and metadata probing.
An actual-algorithm unit controls monotonic time, short sends and EAGAIN to verify
frame serialization, send/reply phase budgets, duplicate/premature responses and
large clock jumps. Native observer tests cover snapshots, state deduplication,
terminal fencing and unmount during persistent delivery. ASan/UBSan use
`detect_leaks=0`; no LeakSanitizer or physical-platform certification is claimed.

This step does not add an extended provider event resource, load telemetry, TCP
keepalive tuning or UDP discovery. Those can supplement this standard Styx probe;
their formats and access policy remain to be agreed.

Проверка выполняется native socket-owner и использует стандартный Tflush к
зарезервированному нерабочему oldtag. Она не трогает файлы и рабочие операции.
Настройка задаётся один раз на соединение; после этого отслеживание автоматическое.
Первый пропуск окна даёт подозрение, корректный ответ — восстановление, достижение
порога — завершение соединения по timeout policy. Отдельно различаются задержка
отправки и ожидание ответа. Единственный probe удерживает теги до Rflush, поэтому
повторные окна ожидания не создают дополнительных незавершённых запросов.
Изменение настройки не забывает уже отправленный probe. Существующий журнал и
почтовые ящики получают уведомления без изменения формата; права не расширяются.
Служебный ресурс событий провайдера и телеметрия нагрузки остаются следующим уровнем.
