# Provider-owned launch discovery / Адрес запуска от провайдера

Native VFS providers can now advertise an owner-side launch address themselves.
`dvm_launch_route_discover(group, path, local_role, spec)` selects one permitted
node using caller-local-first ordering, asks that exact pinned provider for its
address, and submits to its native owner service. It never downloads executable
images or resolves the visible path a second time. Refusal after selection does
not cause a lower-side retry.

Провайдер сам сообщает адрес своего сервиса запуска. Выбранный источник удерживается
до завершения запроса; переименование или перестановка bind не перенаправляет уже
выбранный запрос. Код исполняется на стороне владельца со своим окружением.
Это native-механизм; сетевое согласование адресов ещё не реализовано.

## Native API

| API | Ownership and contract |
| --- | --- |
| `dvm_provider_create_routed(ops, state, capture, fixed_root, native_flags, launch)` | Optional native callback; old ProviderOps table and constructors remain compatible |
| `dvm_provider_launch(provider, relative)` | Borrow pinned provider and canonical relative name; return owned address or NULL |
| `dvm_launch_route_discover(group, path, local_role, spec)` | Select, discover, submit; return independently owned acknowledged ticket or NULL |
| `dvm_launch_address_guard(inner, check, context, release)` | Consume inner on every outcome; adopt context/release only on success |

The callback receives native state and a canonical relative node name (`.` for
its captured root). It returns an owned immutable address whose module label is
chosen by the owner. No visible alias, system nickname or metadata constitutes
execution authority. The owner still reviews/prepares the request and supplies
its actual execution resources.

`DVM_PROVIDER_CANCEL_LAUNCH` (8) promises cancellation-safe native discovery.
It requires PATH support and a launch callback. An active cancellation scope
refuses unsupported callbacks before invoking them; PATH alone is insufficient.
Pre-cancellation also refuses before callback invocation. A direct callback result
remains owned by the caller if cancellation races its return. Routed discovery
frees such a result before queue admission. An already acknowledged ticket remains
valid if cancellation races acknowledgement.

Capture must explicitly recreate a scoped callback; a child factory does not
blindly inherit its parent's callback or authority. Legacy providers may continue
to use explicit `dvm_launch_route_submit` locators, or advertise no launch address.
Host anchors without an advertisement refuse discovery.

## Deferred resolution / Отложенный запуск

`dvm_launch_route_resolve(group, path, local_role)` returns an owned reusable
address, or NULL. It performs the same caller-local-first selection, metadata
check and provider discovery as immediate routing, but needs no launch arguments
or terminal grants. It does not submit a request, reserve admission quota, create
a worker, open code bytes or authorize execution. Resolution happens on an already
native caller outside managed admission, under its current cancellation scope.

```c
DVM_LaunchAddress *address = dvm_launch_route_resolve(group, "/app.dis", local_role);
/* The caller namespace may now change or retire. Inputs for startup can be prepared later. */
DVM_LaunchTicket *ticket = dvm_launch_address_submit(address, &spec);
/* Keep the address for further independent requests, or free it after joining its users. */
dvm_launch_address_free(address);
/* A non-NULL ticket is separately owned and must eventually be freed. */
```

The selected native capability and owner label remain stable across later binds;
a fresh resolution sees the new namespace. Addresses retain no caller namespace
or temporary selection plan. A filtered address retains its exact source mapping
and live policy context, so revocation still blocks later submissions. Ordinary
source metadata is not polled on each use: the owner independently accepts or
refuses each request. A held address cannot revive a closed service.

One immutable address can be borrowed by concurrent native submitters, each with
its own arguments and requester ticket. Owner quota still limits outstanding
requests; releasing one request permits another submit without repeating provider
metadata or advertisement callbacks. Cancellation of one operation does not poison
the address. Late cancellation during resolution disposes its staged address;
after an acknowledged submit the ordinary ticket contract applies.

Отдельный resolve сохраняет владельца выбранного модуля, а не имя для будущего
повторного поиска. Можно сначала разрешить имя, затем подготовить аргументы и
несколько раз подать независимые запросы. Перестановка bind влияет на новый
resolve; отзыв прав экспортирующего view проверяется и на уже полученном адресе.
Это native-основа будущего binding типизированного модуля, а не готовый удалённый
`Module` для обычного `load` и не новый формат сетевого обмена.

## Filtered exports and changing permissions / Права экспорта

Views translate the selected virtual node into its exact mapped source and ask
that source for an address. Hidden nodes, synthetic parents, descendants of exact
file maps and maps lacking READ|EXEC refuse before the source callback. This export
ceiling is distinct from ordinary data reading and standard Inferno Dis loading.
Mixed views support advertised sources while refusing unsupported inner callbacks;
one unsupported source does not disable every other source's scoped discovery.

A returned view address owns a live policy guard. Every submit checks the current
policy for the same immutable mapping without provider I/O or namespace lookup.
Revocation blocks an acquired address; restoring permission permits it again.
Unmounting or releasing the original provider wrapper does not invalidate the
retained capture: source mappings, policy context and native provider references
remain alive until the last guard is released. Journal shutdown is not revocation.

Guard checks are bounded native operations: no guest execution, blocking waits or
provider I/O. Context release is native and nonblocking. Guard chains are freed
iteratively, including deep chains, rather than recursively. The generic guard
core belongs to base VFS and does not force a launch service/VM dependency on data
exports. Addresses, endpoints and callbacks are never serialized as pointers.
No Styx message or S-record format is changed.

## Verification and limits / Проверка и границы

The discovery tests cover rebased capture, exact-file/tree maps, changing policy,
source/context lifetime, mixed cancellation promises, pre/late cancellation,
5,000 nested guards and all 17 observed allocation-failure positions. Each failed
allocation must drain ownership and allow a subsequent successful request.

All 40 interpreter/JIT provider-module scenarios now use deferred provider resolution
instead of a fixture-specific destination table. The caller namespace retires
before submit; two additional variants replace its root with the opposite source
before submission, while parent/child application and dependency calls remain
on the originally selected owner. They execute real Dis applications
and dependencies through two real native owner services on one host. They do not
simulate two networked VMs. Standalone export linking remains independent of the
launch VM/service implementation.

The native SDK has 737 tests. Deferred resolution adds concurrent request/payload
isolation, exact owner queue/quota observations, namespace/source retirement,
pre/late cancellation and all four observed resolution-allocation failures.
All 737 native SDK tests and five affected runtime tests pass normally and with
ASan/UBSan. The runtime cases cover launch import, Unibus launch,
provider modules, launch context and export trees. The retained-resolution
extension adds two native cases and four real Dis executions to the previous
provider-discovery coverage. Private negative controls disabling late-cancel
cleanup or caller-local ordering fail with their intended semantic diagnostics;
compile failures, crashes and timeouts do not count. LeakSanitizer is unavailable
in this environment; explicit allocation and provider-lifetime checks remain on.

Automatic remote Styx address exchange, ordinary `load` to remote typed calls,
and general `.m` marshalling are still unfinished. This native advertisement is
one boundary needed for those paths, not a claim that remote module calls are
already transparent. See [routing](LAUNCH-ROUTE.md),
[code lookup](CODE-LOOKUP.md) and [actual module execution](PROVIDER-MODULES.md).

[Native interface admission](MODULE-INTERFACE.md) adds an optional copied Inferno
name/signature contract to each independent submission. The owner must validate
its actual exports before preparing a typed request. Resolved addresses remain
reusable for different required subsets; no code or value codec is downloaded.

The [retained process contract](MODULE-INTERFACE.md#contract-after-take-and-during-load--контракт-после-передачи-окружения)
now survives request/take and parent/child startup. Exact named-target loads
recheck their actual parsed exports before ordinary linking; this adds no wire
representation, provider lookup or general remote invocation mechanism.

[Explicit local object binding](MODULE-INTERFACE.md#explicit-object-binding--явная-привязка-объекта)
now checks complete requirements on an already loaded Module and invokes the
original Inferno linker on that exact object. It does not infer implementation
identity from a load-name match, repeat VFS discovery or serialize VM pointers.
