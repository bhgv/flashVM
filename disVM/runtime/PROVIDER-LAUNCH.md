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

All 36 interpreter/JIT provider-module scenarios now use provider advertisements
instead of a fixture-specific destination table. They execute real Dis applications
and dependencies through two real native owner services on one host. They do not
simulate two networked VMs. Standalone export linking remains independent of the
launch VM/service implementation.

The native SDK has 735 tests. Five affected dedicated runtime cases cover launch
import, Unibus launch, provider modules, launch context and export trees. All 735 native SDK tests and all five affected runtime tests passed normally
and with ASan/UBSan. Private negative controls disabling live policy or scoped
launch support failed with the intended semantic diagnostic. LeakSanitizer is unavailable
in this environment; explicit allocation and provider-lifetime checks remain on.

Automatic remote Styx address exchange, ordinary `load` to remote typed calls,
and general `.m` marshalling are still unfinished. This native advertisement is
one boundary needed for those paths, not a claim that remote module calls are
already transparent. See [routing](LAUNCH-ROUTE.md),
[code lookup](CODE-LOOKUP.md) and [actual module execution](PROVIDER-MODULES.md).
