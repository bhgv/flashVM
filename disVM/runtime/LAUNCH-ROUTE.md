# VFS selection to launch owner / Из VFS к владельцу запуска

[`route.h`](vfs/launch/route.h) connects a caller-relative VFS selection to a real
native submit capability. It sends one ordinary launch request to that owner;
it does not read/download a module image or execute code during lookup/routing.

Native-маршрутизатор выбирает разрешённый узел по VFS вызывающего, получает адрес
владельца выбранного source и один раз подаёт запрос. Клиент сначала рассматривает
свою сторону, провайдер — свою. Address указывает на submit-only capability и имя
модуля у владельца; строка system ID сама по себе такой capability не становится.

## Immutable address / Неизменяемый адрес

| API | Contract |
| --- | --- |
| `dvm_launch_endpoint_clone(endpoint)` | Independent owned submit wrapper/NULL; closed services refuse, caller keeps its original wrapper |
| `dvm_launch_address_create(endpoint, module)` | Copy a nonempty bounded owner-side logical module label and clone the endpoint; owned address/NULL |
| `dvm_launch_address_submit(address, spec)` | Submit copied caller grants/arguments through that address; owned requester ticket/NULL |
| `dvm_launch_address_free(address)` | Consume address after its users join; does not close service or revoke admitted work |

The address is immutable and may be borrowed by concurrent submitters. The caller
must join them before free. Closing/freeing the service owner makes remaining
addresses unusable for new work while their independent native pins keep closed
service storage valid. Retiring an address or the original endpoint does not
revoke an already acknowledged ticket.

`spec.module` and `spec.expected_provider` are intentionally ignored by address
submission: the owner-chosen label and the actual endpoint's configured service
association supply both. The caller still supplies its granted terminal root,
trusted terminal association, policy, optional FlashVM grant and bounded opaque
startup bytes. Input/grant validation remains the existing launch contract.

```c
DVM_LaunchAddress *address = dvm_launch_address_create(endpoint, "owner/module");
/* endpoint stays owned by the caller; address owns an independent wrapper. */
DVM_LaunchTicket *ticket = dvm_launch_address_submit(address, &spec);
dvm_launch_address_free(address);
/* ticket, if non-NULL, remains independently owned by the requester. */
```

Labels are logical names, not implicitly opened host paths. Native addresses,
endpoints, providers and launch structures must never be serialized as pointer
bytes. This adds no wire tags, endpoint token encoding or authentication scheme.

## Routed submission / Маршрутизация запроса

```c
DVM_LaunchTicket *ticket = dvm_launch_route_submit(group, path, local_role,
    &spec, locate, native_context);
```

The native `locate(context, selected, metadata)` callback receives exactly one
already selected nondirectory node. Its borrowed plan pins the actual provider or
anchor and relative name through callback, submit and temporary address retirement.
The callback returns one **owned** native address, which the router always frees.
It may translate the caller's visible alias into a different owner-side label.

`locate` is the plug-in boundary for a trusted source/transport adapter. It routes
by the selected capability, not by an ambient pathname or an identity string as
permission. It must not copy, mutate or retain the plan, run a guest, enter
managed admission or bypass the source owner's policy. Native I/O may block
outside locks and must consume the current cancellation scope. The callback and
context stay live through routed submit and return normally.

Этот callback — общий native стык адаптера source/transport, а не специальная
клиентская программа для каждого приложения. Автоматическое получение адреса
через сетевой provider ещё предстоит подключить к этому стыку. `.m` остаётся
описанием обычного typed-интерфейса, без добавления индивидуальных wire-клиентов.

Routing validates caller grants/argument bounds before provider metadata work.
It builds a [caller-local plan](CODE-LOOKUP.md), selects acknowledged metadata and
rejects directories. Under a native cancellation scope all candidates must opt
into cancellable metadata before any callback can run. Selection treats opaque
errors as final, not absence; an explicit existing `MAUTORWX` selection policy
remains effective. No selected visible node, lookup refusal, a directory or an invalid request
reaches the locator.

The selected provider/anchor is retained across namespace changes. If another
thread replaces bind/mount while the locator runs, this request still uses its
old selected capability; a later request sees the new namespace. Source contents
and owner policy remain live. No tree or module image snapshot is made.

## Failure, cancellation and authority / Ошибки, отмена и полномочия

- Pre-cancellation performs no lookup/locator/submission.
- Cancellation during address acquisition disposes of that address without
  submitting work. Native allocation/setup failure returns NULL with no partial
  reservation, and does not poison later requests.
- Once the queue acknowledges a ticket, a later racing cancellation does not
  discard/cancel it. The requester retains its ordinary explicit cancel authority.
- Locator refusal, owner close, quota refusal and post-selection failures do not
  retry another side, label or implementation. No automatic replay is introduced.

A ticket is **admission**, not permission to execute. Metadata visibility is not
an execute grant. The selected owner still reviews/authorizes the request, prepares
its permitted live namespace and takes one independent environment. TAKEN does
not prove code/init ran; owner close still denies unfinished work under existing
rules. The existing preparation gap, quotas and cancellation mechanisms are reused.
No additional quota is imposed on Unibus application traffic queues.

The API is native-only, outside managed admission. It supplies no cooperative
service loop, guest pause, global stop or foreign managed pointer. A future managed
adapter must use its ordinary native I/O/cancellation boundary.

## Actual verification / Реальная проверка

Native tests route client-local/provider-local requests into two independent real
owner services. They check copied binary arguments, overwritten caller destination
fields, copied owner labels, exact-file aliases, final locator refusal, missing/
denied/directory nodes, unsupported cancellation promises, pre-cancel, cancellation
in the locator, bounded admission and lifetime after address/original endpoint
retirement. Closed addresses cannot revive a service.

A deterministic native gate replaces live root bindings after source selection.
The held request must still reach the original owner; the next request reaches the
new owner. A linker gate requests cancellation after actual queue acknowledgement;
the real returned ticket remains pending/usable. Native allocation observation
refuses all17 actual routing/address/submission positions, requires complete quota
and allocation drain, then successfully submits again.

The existing36 interpreter/JIT provider-module executions now go through caller
VFS → address → selected owner queue → blocking native handler → prepare/take →
controller startup → actual Dis application/dependency calls. Client-source code
uses the client owner's execution root; provider-source code uses the provider
owner's root. These are two native fixture services on one host, **not** two
networked VMs. Unrelated namespace/data behavior and actual image/handle retirement
checks remain enabled.

Private negative controls must fail semantically when caller labels replace owner
labels, acknowledged tickets are discarded after cancellation, or routing re-resolves
the live path after the locator. Compile errors/crashes/timeouts do not count.

Native provider-side address discovery is now available through
[`dvm_launch_route_discover`](PROVIDER-LAUNCH.md), including filtered exports and
live policy guards. Transparent ordinary `load` → remote typed calls, authenticated
network capability exchange and general `.m` marshalling remain separate work. This change implements the native selection-to-admission
boundary without guessing a new Styx/S-record message dialect.

The full native SDK passes733/733 tests ordinarily and with ASan/UBSan. Four
affected dedicated runtime cases also pass in both builds, including all36
provider-module executions. Final route cases additionally cover true absence
fallback in both directions and pass normally/with sanitizers. LeakSanitizer is
disabled in this restricted environment; explicit native allocation/provider
ownership checks remain enabled. Imported Inferno libraries remain byte-identical.

The current provider-discovery extension adds two native tests (735 total) and
replaces the module fixture locator table with source-owned advertisements.
See [provider discovery](PROVIDER-LAUNCH.md) for the current contracts and coverage.
