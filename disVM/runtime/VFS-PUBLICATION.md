# Limbo publication capabilities / Публикация из Limbo

Copyright (c) 2026 bhgv

Runtime0.135 exposes the existing [native publication SDK](VFS-EXPORT.md) to local
Limbo guests through [Vfsexport](../module/vfsexport.m) and the pure Limbo
[Vfspublish](../module/vfspublish.m) request reader. This is an explicit extension;
Inferno Sys.m, upstream source, mount/export flags and Dis ISA remain unchanged.
There is no network listener or wire capability encoding in this checkpoint.

Теперь владелец может принять запрос из другого гостевого потока, получить его
настоящий локальный PID, подготовить и выдать другое дерево. Дерево после выдачи
работает как обычный VFS provider: open/read/write/list и mount/bind общие.
Никакого вызова managed callback на чужом native потоке или принудительной паузы
рабочего потока нет. Обработчик подготовки — обычная функция приложения владельца.

## Authority / Полномочия

| Native type | Authority / Полномочия |
| --- | --- |
| Root | Captured ordinary tree, transferable through typed local channels |
| Owner | Derive endpoint, review/reply, obtain notification source and close publication |
| Endpoint | Submit requests; no review, reply or close authority |
| Ticket | Observe, take one grant or cancel that request |
| Request | Immutable owner-side copied request tied to its exact publication |

All five are opaque **native-created exact types**. Their public reserved field
has no authority. A Limbo-created record with the same visible fields is refused.
No integer ID, PID or host pointer can be substituted for any of these capabilities.
Sharing Owner delegates administrative authority; share Endpoint to delegate only
request admission. A native grant is additionally limited to requested rwx rights.
Aliasing a capability shares its state, rather than creating a new native owner.

Все ссылки принадлежат обычному GC. Пять типов явно допущены как pointer-free
native shells. Их finalizer только передаёт уже выделенную cleanup job native
closer; он не вызывает файловый I/O, чужой VM callback или managed код. Closer
освобождает native tree/request/service после ухода managed borrowers. Никакого
общего останова GC/потоков для этого не вводится.

## Tree API / Работа с деревьями

| Call | Contract / Контракт |
| --- | --- |
| capture(path, index, rights) | One effective signed layer in caller's current namespace; owned Root or nil |
| overlay(base, source, path, rights, flags) | New immutable tree; nil base starts empty, nil source hides virtual prefix |
| info(root) | Current status0/-1, rwx mode and directory/file kind before mount; no permission grant |
| mount(root, target, flags, index) | Ordinary combined mount/bind;0 success,-2 stale,-1 refusal |

capture uses Vfsctl's signed top/bottom indexing. It captures **one explicit layer**,
not a recursive snapshot of every nested bind under that path. Capture other layers
or separately bound child resources explicitly and compose them with overlay.
A captured directory delegates its descendants; a captured file exposes only itself.
Native directories use confined anchors; native exact files retain parent/name,
with the same documented identity limits as the C SDK. Existing provider roots keep
their own stable-bind contract and source journal. rights must be1..7, using
Read4/Write2/Execute1. Later source permissions and actual I/O still decide access.

overlay maps the chosen source root dot to an absolute virtual path. Tree1 delegates
descendants; flags0 publishes only the exact node. Hidden prefixes, substituted
files and synthetic parents use the same native view implementation as the C SDK.
It preserves the base and source. Source caps keep their own rights ceilings, so
an overlay cannot widen a readonly source into a writable source. Every composition
creates a new wrapper; deeply chained views cost proportional native dispatch/stack
work and are not flattened or given an unbounded-depth safety guarantee.

mount uses MREPL/MBEFORE/MAFTER/MCREATE/MAUTORWX and signed MINS from
[Vfsctl](VFS-LAYERS.md). It mutates only the caller's namespace group. ForkNS copies
that group as before; share the group explicitly if changes should be shared.
Captured capabilities are independent of the namespace that originally supplied
them and may outlive the sending guest. Native host OS watchers and credentials
remain outside this interface; this is not a whole-process security sandbox.

## Publication API / Запрос и решение владельца

| Call | Contract / Контракт |
| --- | --- |
| announce(limit, Prepare, nil) | Create owner; every request waits for explicit owner reply |
| announce(limit, 0, direct) | Direct prechosen tree; sharing endpoint explicitly delegates admission |
| endpoint(owner) | Derive request-only endpoint, no administrative authority |
| submit(endpoint, name, kind, rights) | Copy logical name; native code captures actual permanent caller PID; Ticket or nil |
| next(owner) | Exactly one queued Request, nil if none; never causes notification polling |
| describe(request) | Copied review id, actual requester PID, requested kind and rights |
| text(request, offset) | One immutable UTF-8 name byte:1/byte,0/end,-1 invalid |
| reply(owner, request, chosen) | Matching owner/request only; chosen Root or nil deny;1 accepted,0 refused/gone |
| status(ticket) | Nonwaiting: Pending0, Granted1, Denied2, Consumed-1 |
| take(ticket) | One-use transfer of grant; nil pending/denied/consumed |
| cancel(ticket) | Release ticket/grant and quota;1 changed,0 already cancelled |
| close(owner) | Deny pending, refuse new requests; accepted trees survive;1 changed,0 already closed |

The native SDK supplies the queue and decisions; the guest bridge does not duplicate
that state machine. limit1..256 counts **all outstanding tickets**, including consumed
grants until cancel or GC. Retained review cards alone do not occupy requester quota.
Native request names are logical labels, never automatic host path lookups. kind is
only a hint0 unspecified/1 file/2 directory; the owner chooses the actual resource.
No peer string, PID or REMOTE/AUTHENTICATED bit is accepted from Limbo. This local
adapter derives identity natively. A future remote adapter must authenticate peers
before entering the C SDK; it cannot reuse local PIDs as network credentials.

Vfspublish.init loads the native module. Its next(owner) calls native next once and
returns a copied ADT containing handle/id/pid/kind/rights/name. reply uses only the
opaque handle; edited copied fields cannot authorize a different request. A request
card may be retained for later reply. Dropping the final card without answering
causes denial when native cleanup runs. Explicit reply is preferable when immediate
refusal matters. Different requests can be answered out of order; native next is FIFO.
Use info on a granted Root to select a matching file/directory mount target; the
requested kind is merely a hint and need not equal the granted node kind.
A card from another publication cannot answer a request even if IDs coincide.

Concurrent aliases synchronize native ticket take/cancel and service close/reply.
One alias can consume/cancel a ticket for all aliases. After successful take, call
cancel to release its outstanding quota promptly; the returned Root remains valid.
Managed output failures never replay a request, reply or take. A failed take result
publication can leave the native ticket consumed; inspect status rather than
resubmitting an effect. Native counters are diagnostic, not capability authority.

## Automatic notification / Автоматическое уведомление

```limbo
owner := api->announce(64, Vfsexport->Prepare, nil);
notice := api->events(owner);
if (notice == nil || api->mount(notice, "/srv/review", 0, 0) != 0) {
    raise "cannot bind publication notice";
}
```

The target must exist with the ordinary matching file kind. events(owner) returns
an ordinary readonly notification node. Binding it automatically attaches the
existing per-thread namespace watches. A newly pending request updates its source
journal with Content, then native push delivers an ordinary VFS invalidation through
/dev/sysevents to threads whose namespace sees that node. No next/status call,
Limbo polling task or foreign guest execution is needed to publish the notice.
The byte stream of the node is empty; it is a change source, not a request protocol.
Do not interpret its EOF as a closed publication. Request records come from next.

Событие сообщает, что состояние изменилось. После него владелец вызывает next для
**одного** запроса и решает, что выдавать. Несколько уведомлений могут объединяться
обычными правилами journal/inbox; событие не резервирует запрос. Пустой next после
события допустим, например после отмены. Прямой режим не требует review и не выдаёт
уведомление о подготовке. Формат /dev/sysevents остаётся предварительным.

## Owner preparation / Пример обработчика

```limbo
prepare(request: ref Vfspublish->Request): ref Vfsexport->Root
{
    if (request.name != "/project" || request.kind != 2) {
        return nil;
    }
    tree := api->capture("data", 0, Vfsexport->Read | Vfsexport->Execute);
    replacement := api->capture("filtered/input", 0, Vfsexport->Read);
    if (tree == nil || replacement == nil) {
        return nil;
    }
    tree = api->overlay(tree, nil, "/private", 0, Vfsexport->Tree);
    if (tree == nil) {
        return nil;
    }
    return api->overlay(tree, replacement, "/upload", Vfsexport->Read, 0);
}
```

The owner's normal handler takes r:=requests->next(owner), calls prepare(r), then
requests->reply(owner,r,chosen). The requester keeps its Ticket and later takes
the Root to mount in its own namespace. Replace filtered/input by any already
available provider endpoint; this SDK does not implement an antivirus engine.
The source path and requested /project need not match. Access checks and source
I/O remain active after grant. This snippet is an application callback, not a native
callback registration that executes guest code on the submitting thread.

[Complete executable Limbo acceptance example](fixtures/vfs-publication/main.b)
shows cross-thread submission, copied Unicode names, automatic notices, a hidden
secret, substituted file and added service/input, exact-file direct publication,
wrong-owner rejection, cancellation, bounded queue, close, forged records, competing
alias take and child namespace mount isolation.
The [test driver](test_vfs_publication.py) runs interpreter/JIT × both GC observation
settings, plus real root-registration/result-publication refusals, against dynamic
and static builds. Independent host checks preserve private/source bytes, and a
native closer hook verifies all constructed capability states were retired.
Targeted ASan/UBSan instrument the capability/view/publication/provider journals;
the remaining VM/port objects are uninstrumented and LSan is disabled under ptrace.

Wire Styx/9P, network authentication, remote capability encoding, heartbeat,
load-balancing, recursive namespace snapshot capture and running-code migration
are still future work. Existing native SDK remote-adapter entry points remain
available without pretending this local Limbo endpoint is a network service.
