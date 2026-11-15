# Prepared publication trees / Подготовка публикуемого дерева

Copyright (c) 2026 bhgv

Runtime0.134 adds a native C embedding SDK in [vfs/export.h](vfs/export.h).
It separates the request, owner decision and capability grant. A request names
what a peer wants; the owner may refuse or publish a different ordinary provider.
The requested name never becomes an automatic host filesystem lookup.

Runtime0.134 добавляет native C SDK: запрос → решение владельца → выдача capability.
Перехватчик получает peer, logical name, kind и требуемые права. Он может отказать
либо вернуть совсем другой файл/дерево. Выданное дерево — обычный provider:
дальнейшие open/read/write/list и bind используют общие VFS операции.

## Compatibility and preparation option / Совместимость и флаг подготовки

This is an explicit extension, not an Inferno Sys interface change. Inferno
`announce(addr)` creates a listener; `export(fd, dir, flag)` serves a directory in
its namespace. EXPWAIT/EXPASYNC select execution mode, not a preparation callback.
A caller can prepare an Inferno namespace with pctl/bind before export.
See the official [announce](https://inferno-os.org/inferno/man/2/sys-dial.html),
[export](https://inferno-os.org/inferno/man/2/sys-export.html) and
[bind/mount](https://inferno-os.org/inferno/man/2/sys-bind.html) documentation.
No upstream snapshot, Dis opcode, Sys.m frame or reserved Inferno flag is changed.

Наш DVM_EXPORT_PREPARE — отдельная настройка publication, не флаг Inferno announce,
export или mount. Она выбирает путь принятия решения:

| Setting / Настройка | Behavior / Поведение |
| --- | --- |
| PREPARE, native prepare callback | Callback sees request; returns owned chosen provider, refuses, or defers |
| PREPARE, no prepare callback | Request queues for owner next/reply; native notify signals availability |
| No PREPARE | Explicit authorize gate checks request; previously selected direct tree is granted |

Preparation does not disable access control. Direct mode requires authorize;
PREPARE requires a callback or an explicit owner reply before granting anything.
All grants are additionally restricted to the requested rwx ceiling.
Подготовка не отключает проверку прав; отсутствие PREPARE тоже не даёт обхода ACL.

## Composing an ordinary tree / Композиция обычного дерева

DVM_View is an exclusively owned mutable builder. create starts empty/default deny.
map adds/replaces an absolute virtual path, retained source provider and canonical
capability-relative source path. The source root is dot. DVM_VIEW_TREE delegates
its descendants; without it only that exact node is exposed. NULL source hides a
prefix. Paths reject dot-dot, repeated separators, trailing slash and oversize names.
The longest matching mapping wins. Explicit descendant mappings can reopen selected
nodes below a hidden prefix; the hidden parent itself stays inaccessible unless
explicitly mapped. Synthetic directory parents connect otherwise unmapped nodes.

Builder freeze consumes it on **every outcome**, yielding an owned ordinary
DVM_Provider or NULL. Failed map preserves existing mappings and source ownership.
Discard consumes an unused builder. Successful freeze retains all sources until
the final view, stream, binding and in-flight source notice retire. Stable-bind is
promised only if every mapped source provides the same promise. Maximum256 rules.

```c
/* EN/RU: Host owns base/filter; a filter is an ordinary provider with its own policy. */
DVM_View *v = dvm_view_create(peer_policy, peer_context, release_peer);
dvm_view_map(v, "/", base, ".", 5, DVM_VIEW_TREE);       /* r+x tree */
dvm_view_map(v, "/private", NULL, ".", 0, DVM_VIEW_TREE); /* hide prefix */
dvm_view_map(v, "/upload", filter, ".", 6, 0);          /* replace by r+w endpoint */
dvm_view_map(v, "/service/input", other, ".", 6, 0);    /* add another source */
DVM_Provider *prepared = dvm_view_freeze(v);             /* consumes v */
```

This abbreviated example omits error branches: production code must check create,
every map, and freeze, discarding a failed builder. create takes context ownership
only on success. Callbacks are trusted native code, never managed VM callbacks.
Sources may implement files, channels, devices, database records or filtering
streams. The SDK supplies composition and admission, not an antivirus engine or DB.

Каждый child в list проверяется по virtual path; hidden/replaced source entries
не просачиваются и не дублируются. Полностью скрытый исходный batch не считается
EOF: enumeration продолжает до видимых entries или настоящего EOF. После256 подряд
скрытых batches возвращается ошибка, не ложный EOF. Ошибка инвалидирует cursor.

The optional policy callback checks metadata visibility (access0), open, and each
subsequent I/O/list/seek. Changing policy can revoke an already opened stream.
It must be thread-safe, with context alive until release; no VM execution or
namespace reentry. Denials suppress operations, not merely metadata mode bits.
Delegated operations still decide actual access. Precise partial counts/causes are
preserved; diagnostic mode is masked to the view's rights. A filter that consumes
bytes before denying must report its own accurate prefix/certainty.

Source journal changes automatically forward to the view's journal and ordinary
namespace subscribers. No receiver polling, guest pauses or callback execution in
a guest is required. Source-specific resource IDs are removed; change classes are
preserved as provider-wide invalidations with the view's session/generation.
Local policy changes require their owner to report an invalidation; the operation
checks do not rely on having received a notice. Native host file changes do not
magically acquire an OS watcher. Event record format remains provisional.

## Request and grant / Запрос и выдача

DVM_ExportOptions is copied at create; direct is retained. Callback context is
borrowed and must outlive all joined publication operations. limit bounds outstanding
tickets1..256, including granted/reviewed tickets until requester releases them.

| API | Ownership and result / Владение и результат |
| --- | --- |
| publication_create(options) | Owned publisher capability, NULL for invalid/OOM |
| publication_request(pub, request) | Copies request, assigns monotonic ID, returns owned opaque ticket or NULL |
| publication_next(pub, output) | Nonwaiting, exactly one pending FIFO request:1; none:0 |
| publication_reply(pub, id, chosen) | Borrowed chosen provider or NULL deny;1 accepted,0 gone/answered/closed/allocation failure |
| export_take(ticket, grant) | Nonwaiting:0 pending,1 transfers owned grant,2 denied,-1 consumed/invalid |
| export_release(ticket) | Cancels/unlinks, consumes requester ticket; late reply fails |
| publication_close(pub) | Consumes owner capability, refuses pending requests; existing grants survive |

The function names in this table carry the dvm_ prefix. A prepare callback returns
1 with an **owned** chosen provider,0 for deferred owner review,-1 for denial.
The SDK consumes its chosen reference. A successful reply wraps the chosen tree
with a separate rights ceiling; failure never silently exposes a direct fallback.
The publisher-local integer ID permits owner review, **not** requester redemption.
Only the opaque native ticket authorizes take; pointers never go on the wire.
Requesters serialize operations on their own ticket. Different tickets and owner
replies synchronize internally. Source I/O/destructors and user callbacks run
outside the publication mutex. Stop/join submitters and owner operations before
close; tickets may outlive close. Concurrent reply/cancel is supported.

Без PREPARE выдаётся ранее выбранное дерево после authorize. С PREPARE владелец
сам выбирает provider, не обязан отдавать requested name. Он может заранее
подготовить namespace со своими bind, построить view со скрытыми/добавленными/
подменёнными узлами и выдать результат. Получатель не видит исходные host пути.
Native notify сообщает о pending request; next выдаёт ровно один запрос.
Это не кооперативная точка исполнения и не остановка рабочего потока.

Remote requests require REMOTE plus AUTHENTICATED provenance **set by a trusted
transport after identity verification**, never copied from client flags/peer text.
The peer string alone grants no authority. Policy decides what that peer receives.
Local peer labels also require a trusted caller boundary. This SDK accepts such
adapter requests, but does not itself implement authentication or a Styx listener.
The local Limbo adapter added in runtime0.135 uses opaque owner/endpoint/ticket
capabilities and deferred review; see [guest API](VFS-PUBLICATION.md). It never
executes managed callbacks on an arbitrary submitting native thread. A remote
Styx adapter remains pending.

## Native roots and combined mount/bind / Native roots и mount+bind

export_native captures a borrowed native resource plan into an owned provider.
Directory roots pin directory identity. Exact-file roots retain a parent/name,
not an Inferno Chan.walk identity across replacing that child. Exported descendant
paths use an opt-in confined POSIX anchor: component traversal rejects dot-dot and
symlinks, including final symlinks. Directory listing hides symlinks. Native access
checks still consult effective OS identity/ACLs; virtual peer isolation comes from
explicit view/policy. This is not whole-process sandboxing or a hard-link boundary.
Ordinary anchors retain their previous native semantics. Windows confinement
returns UNSUPPORTED; only Linux AMD64 was executed here.

stack_mount(group, provider, target, flags, index) combines provider installation
and bind in one revision-checked stack edit. Provider remains borrowed; target and
kind checks use the ordinary stack path. MINS signed indexing, BEFORE/AFTER/REPL,
MCREATE, MAUTORWX and file/subtree overrides follow [Vfsctl](VFS-LAYERS.md).
Returns1 success,0 stale,-1 error; no partial namespace publication. Existing FDs
retain their old resource. Group lifetime must be held by the native caller.
No global provider IDs or nsadmin authority are introduced.

## Acceptance / Проверки

Native tests exercise real native roots, filtered writes, hidden/replaced/added
entries, synthetic parents, hidden first directory batches, live revocation,
source journal forwarding, combined mount/MINS and ordinary VFS reads. Independent
host checks verify original/private bytes remain unchanged after symlink escape
attempts. Publication tests cover direct/prepared/deferred paths, authentication
provenance rejection, rights ceilings, one-use grants, queue bounds, FIFO, close,
late replies and100 real reply/cancel races. Another native worker emits500 source
notices while100 projected views subscribe and retire. Targeted ASan/UBSan runs
instrument SDK/VFS sources; the shared port archive is uninstrumented and LSan is
disabled in this ptrace environment. Runtime0.135 adds the [local guest publication API](VFS-PUBLICATION.md). Full Styx
transport, heartbeat, remote execution/migration and balancing remain future work.
