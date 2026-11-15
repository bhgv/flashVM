# Provider changes and state refresh / Изменения провайдера и перечитывание

Copyright (c) 2026 bhgv

Runtime 0.129 connects a bounded native change journal to each retained provider.
It implements notification reception from native adapters, independent observers,
selective reread and safe acknowledgement across concurrent changes/reconnection.
It does not implement a Styx transport, heartbeat socket, background refresh worker,
load sampler, file-stack balancing or a new guest API. These remain separate work.
Runtime 0.129 подключает native журнал к retained provider: приём уведомлений от
native adapters, independent observers, selective reread и безопасный ack.
Styx/heartbeat transport, background worker, sampler, балансировка и guest API
этим шагом не реализуются.

## Event classes / Классы событий

| Mask / Маска | Meaning and refresh target / Значение и цель перечитывания |
| --- | --- |
| RIGHTS | Access policy may have changed; reread relevant resource/policy / Возможно изменение прав ресурса/policy |
| NAMESPACE | Directory/resource membership may have changed / Возможно изменение состава каталога/resources |
| CONTENT | Resource content/version/attributes may have changed / Возможно обновление данных/версии/атрибутов |
| CONNECTION | Availability, planned draining or disconnection observation / Доступность, planned draining, наблюдение отключения |
| LOAD | Provider-side CPU/memory/queue observations / Наблюдения CPU/памяти/очередей провайдера |
| NETWORK | Observations about this client's network path / Наблюдения пути этого клиента к провайдеру |

Declarations and native contracts are in [changes.h](vfs/changes.h). A zero kind
mask means unspecified change and broadens to ALL. Unknown kind/flag bits refuse
publication. CONNECTION/LOAD/NETWORK are provider-scoped. RIGHTS/NAMESPACE/CONTENT
may name a single opaque resource, with an explicit RESOURCE flag; resource zero
is valid. The identity can represent a provider-local qid/path token and carries
no memory address or access authority.
Нулевой kind mask означает ALL; неизвестные bits отвергаются. Для resource scope
нужен отдельный flag, ID=0 допустим. Идентификатор не является полномочием.

The provider object routes the notification: obtain its borrowed journal with
`dvm_provider_changes` while retaining the provider. Aliases and FORKNS copies
of that factory share the journal but do not share observer cursors. Another
factory, even with an identical display label, owns a separate journal.
Routing задаёт retained provider object, не display path. Aliases/FORKNS разделяют
journal этой factory; cursor каждого observer остаётся отдельным.

Events do not relax the stable-bind identity promise, redirect opened resources,
or invalidate loaded code automatically. Existing provider/module limitations
remain explicit. События не ослабляют stable-bind promise, не подменяют открытый
ресурс и не сбрасывают автоматически загруженный code.

## Sessions, loss and coalescing / Sessions, потери и объединение

A new journal starts at session1/generation1 with every class requiring refresh.
`dvm_changes_reconnect(changes, expected_session)` atomically allocates the next
local epoch only when the expected epoch matches. This changes bookkeeping, not
the network connection or open fids. Old-session reports are ignored; future-session
reports are rejected until the trusted connection owner establishes that epoch.
Sessions never wrap. A restarted provider object has a different retained identity.
Новый journal начинается с session1/generation1 и ALL dirty. Reconnect API меняет
только epoch учёта, не соединяет socket и не переоткрывает fid. Проверка expected
не даёт stale reconnect заменить новую epoch. Старые session игнорируются.

Within a session, each generation denotes one immutable complete source report.
Identical/late reports do not advance state. If a generation jumps, missed classes
cannot be known: the accepted notification broadens to provider-wide ALL. This
prevents a later CONTENT report from hiding a lost RIGHTS report. A source can
coalesce several changes into one report before assigning its next generation.
One connection owner must serialize remote and local reports into that stream;
independent counters must not be mixed. A future adapter must detect gaps in its
remote wire sequence before mapping reports into a locally sequenced stream.
Exhausted generation requires a new epoch; UINT64_MAX cannot wrap to zero.
Generation — один immutable report со всем kind mask. Duplicate/late reports не
меняют state. Gap означает неизвестные пропущенные classes: ALL provider-wide.
Объединять события можно до назначения новой generation. Wrap запрещён.

The journal retains at most32 events. Each class also has a persistent latest
version, so evicting history never loses its dirty state. Pending events retain
single-resource scope only when all relevant unseen history is available and
names the same resource. Mixed resources, a generation gap, session change or
insufficient history force conservative provider-wide scope.
Journal ограничен32 events. Latest version каждого class сохраняется независимо
от history. Narrow scope допустим только при полной relevant history и одном ID;
иначе scope расширяется. Потеря detail не превращается в потерю invalidation.

## Refresh protocol / Перечитывание

Each consumer owns a zero-initialized `DVM_ChangeCursor`. `dvm_changes_pending`
returns selected dirty classes without acknowledging them. Observation is native,
allocation-free and performs no provider I/O.
Каждый consumer владеет своим cursor. Pending ничего не подтверждает и не делает I/O.

`dvm_changes_refresh` performs these steps:

1. Capture a ticket with session, dirty classes and their exact versions.
2. If clean, return without calling the provider.
3. Invoke the supplied native refresh callback once, outside the journal mutex.
4. On failure, preserve the cursor. If session changed, return NOT_FOUND without ack.
5. On success, acknowledge only the captured class versions; later events stay dirty.

Callback получает ticket и делает требуемые запросы к тому же provider через
существующие info/open/read/list operations либо свой negotiated status endpoint.
Callback под journal lock не вызывается и может сам опубликовать новое событие.
Ошибки и новая epoch не очищают cursor; новые версии остаются dirty.

The callback stages consumer-owned data. Publish it only after a successful
refresh result, retaining its session/version tag. This API is an acknowledgement
mechanism, not an atomic shared cache or a server snapshot guarantee. The backend
query must return state at least as current as the ticket, or report refusal/staleness.
Sharing one cursor concurrently requires external serialization. The helper makes
one attempt, never loops until a busy provider becomes quiet.
Callback готовит consumer-owned data. Публиковать его после успешного refresh,
с session/version tag. Это ack mechanism, не atomic cache/server snapshot. Backend
должен вернуть state не старее ticket либо отказать. Shared cursor требует внешней
сериализации; helper не делает busy loop. Deadline/cancellation задаёт backend query.

For example, if CONTENT generation2 arrives during a refresh of generation1,
only generation1 is acknowledged. Another refresh still sees generation2. A failed
query of an offline provider leaves the event pending; a new connection epoch
requires rereading all selected classes before that observer becomes clean.
Пример: CONTENT generation2 во время refresh generation1 остаётся pending. Offline
query не подтверждает событие; новая epoch требует полного обновления observer.

## Connection to chunk I/O / Связь с chunk I/O

`DVM_IoResult` can now attach explicit session, change classes and optional resource
to its existing CHANGED/generation hint. The retained factory wrapper forwards that
hint to its journal after the underlying operation. Invalid/stale hints do not
alter acknowledged bytes or operation status. Legacy hints without an explicit
session remain diagnostics; attaching an old reply to the current epoch would be
unsafe. The [precise chunk contract](CHUNK-IO.md) stays intact.
DVM_IoResult несёт explicit session/classes/resource. Wrapper передаёт hint в journal,
не меняя результат I/O. Unscoped legacy hint остаётся diagnostic: старый reply нельзя
приписывать новой epoch. Invalid/stale hint не превращает успешный I/O в отказ.

Receiving a hint performs no stat. An explicit consumer refresh can later query
state because of that hint, independently from the successful I/O path. Failed
I/O still attempts at most one diagnostic info query under the existing chunk
contract. Notification refresh and failure diagnostics are different operations.
Получение hint не делает stat. Позднее consumer может перечитать state по событию.
Failed I/O сохраняет свой единственный diagnostic info. Лишнего per-success stat нет.

## Future wire and scheduling work / Будущий transport и scheduling

A heartbeat can carry these classes/generation in a negotiated envelope or expose
notifications through a control/event file. Standard Styx layouts and reply/tag
matching must remain unchanged. A local detector reports unexpected disconnect;
a remote provider can announce planned draining before disconnecting. A hint grants
no rights, does not turn refusal into EOF, and never permits replay of uncertain
writes. No network reconnect or automatic file switching is added here.
Будущий heartbeat использует negotiated envelope/control file, не меняет standard
Styx frames. Внезапный disconnect наблюдает клиент; planned draining может сообщить
сервер. Hint не даёт прав, не разрешает replay и не переключает открытый файл.

LOAD and NETWORK currently classify invalidation only. The refresh callback decides
what status files to read; no metric schema, TTL, sampler or least-load policy is
implemented. Future balancing must distinguish provider CPU/queue state from
client-path latency/loss and retain sample timestamps/validity. It must first respect
resource access/availability and preserve confirmed-prefix/uncertain-tail semantics.
Existing MAUTORWX and native/module cache rules are unchanged. File, server input,
queue and compute endpoint can retain the same guest file interface.
LOAD/NETWORK пока только classes; metric schema/TTL/sampler/балансировки нет. Будущая
policy различает CPU/очередь сервера и задержки/потери пути клиента, хранит timestamp
и validity. Права, availability и I/O acknowledgements остаются обязательными.

## Verification / Проверки

[Native unit](vfs/test_changes.c) tests selective/failed refresh, copied independent
cursors, notification inside refresh, epoch replacement during refresh, gaps,
duplicates, resource zero/mixed scope, history overflow, generation exhaustion
and4999 reports from a real producer OS thread concurrent with observer refreshes.
[Real guest fixture](fixtures/provider-events/main.b), [host provider](test_provider_events.c)
and [driver](test_provider_events.py) verify ABC bytes, zero success stats, typed
hint routing, actual factory reread, races, offline refusal, epoch recovery and
exactly-once factory lifetime in interpreter/JIT and both polls. Recovery here is
native state-query simulation, not a network outage acceptance test.
Unit и guest проверяют реальные native paths; offline/recovery — simulation state
query, не acceptance сетевого reconnect. Upstream snapshots и public Sys ABI не меняются.
