# Inferno interface checks before launch / Проверка интерфейса до запуска

A native launch request can carry an immutable snapshot of the caller's required
Inferno function names and exact unsigned 32-bit link signatures. The owner must
confirm matching exports before environment preparation begins. This is the same
name/signature compatibility rule used by Inferno's `linkm` in
[`upstream/libinterp/link.c`](../upstream/libinterp/link.c): every requested member
must exist with the same signature. Extra exports and declaration order do not
prevent a match. No new Limbo type hash, DIS opcode or shared ABI ID is introduced.

Клиент передаёт требования интерфейса, а не исполняемый код и не свои frame pointers.
Владелец проверяет экспорты своей реализации. Несовместимый typed-запрос остаётся
в `PENDING` для явного решения владельца; подготовка его окружения не выполняется.
Выбор владельца по VFS и порядок «сначала своя сторона» остаются прежними.

## Native API

| API | Contract |
| --- | --- |
| `dvm_launch_contract_create(symbols, count)` | Owned sorted copy of names/signatures; NULL on invalid/duplicate/bounds/OOM |
| `dvm_launch_contract_clone(contract)` | Independent owned immutable copy; NULL on NULL/OOM |
| `dvm_launch_contract_count(contract)` | Immutable member count; zero for NULL |
| `dvm_launch_contract_symbol(contract, index, out)` | Borrow one sorted name, copy its signature; failed output remains unchanged |
| `dvm_launch_contract_matches(required, available)` | Required-subset match; neither input consumed, no allocation or I/O |
| `dvm_launch_contract_free(contract)` | Consume exclusive owner after readers join; NULL allowed |
| `dvm_module_contract_imports(imports)` | Copy a real parsed caller Import table into a native contract |
| `dvm_module_contract_exports(module)` | Copy a real loaded Module's export table; no Module/Type/code pointer escapes |
| `dvm_launch_request_contract(request)` | Independent owner-review copy of requirements; NULL for untyped/invalid/OOM |
| `dvm_launch_request_check(request, exports)` | Accept a match only while PENDING and not cancelled; no environment acquisition |

Public native declarations live in [`contract.h`](vfs/launch/contract.h) and
[`ticket.h`](vfs/launch/ticket.h). Actual VM metadata adapters live separately under
[`module_interface`](module_interface/contract.h); the SDK contract/check code has
no dependency on VM structures, parser execution or managed roots.

## Request and owner lifetime / Владение

```c
/* Startup controller holds its actual caller Module while copying this table. */
DVM_LaunchContract *required = dvm_module_contract_imports(imports);
if (required != NULL)
{
    spec.contract = required;
    DVM_LaunchTicket *ticket = dvm_launch_address_submit(address, &spec);
    dvm_launch_contract_free(required); /* A successful request owns a separate copy. */
    /* Consume the independently owned ticket after handling success/refusal. */
}
```

Check `required` before submission: an allocation/bounds failure must not silently
turn a typed request into an untyped one. The native request constructor copies
all requirements before publishing requester/provider capabilities. `description.typed`
distinguishes an untyped request from a failed review-copy allocation, including
an explicitly supplied empty contract.

The native owner loads/selects its own implementation and holds that version
while copying exports. It then calls `dvm_launch_request_check(request, exports)`
before `dvm_launch_request_prepare`. A refused check leaves PENDING and does not
query provider roots. A successful check records one-way acceptance under the
request lock; preparation still independently checks owner identity, granted
roots, cancellation and the ordinary one-use state transition. Check after
PREPARING/READY/TAKEN/cancellation refuses. No execution authorization follows
from a matching hash.

Keep the implementation version chosen for the check through actual binding/use;
a metadata snapshot alone does not pin future code loaded from a changing path.
Named-target loading now repeats the interface check on the actual parsed Module,
as described below. The caller is responsible for pinning valid parsed
Import/Module storage while an adapter reads it. Returned native snapshots own all names/signatures and may
outlive source modules, client buffers, tickets and requests independently.
Readers may share immutable contracts; their owner must join readers before free.

## Bounds and compatibility / Ограничения

The new native snapshot API allows at most 1,024 members and 256 UTF-8 bytes per
member name including NUL. Names must be nonempty; duplicates refuse even when
the signatures agree. Signatures are exactly 0..0xffffffff, including the high
bit of a negative native `Import.sig`. Wider values refuse instead of narrowing.
A non-NULL empty requirement matches any non-NULL export table but still requires
explicit owner confirmation when submitted as a typed request.

Zero-initialized `DVM_LaunchSpec` keeps the existing untyped path. The native
spec/description structures gain trailing fields and native consumers must rebuild;
ordinary `.dis` encoding, `.m` signatures and ProviderOps remain unchanged.
Existing Styx launch-request adapters copy the optional contract through the same
constructor. No contract pointers or native structure bytes are a wire format.
No transport, Styx message, S-record tag or tag-0 negotiation is added here.

## Evidence and remaining work / Проверка и следующий этап

Native tests cover copied storage, reordered/subset exports, missing members,
intact unsigned32 bits, bounds, duplicates, empty interfaces, owner review copies,
cancelled/legacy requests and the preparation gate before provider I/O. Existing
constructor/service/preparation/handoff allocation campaigns now also run with a
typed contract, including explicit snapshot create/clone refusal and recovery.
Private negative controls removing the preparation gate or ignoring signature
bits fail with the intended semantic diagnostic; compile errors/crashes/timeouts
are not accepted as detection.

All 50 actual interpreter/JIT provider-module scenarios use a real compiler Import
table and owner exports parsed from a real `.dis`. A separately compiled
[`wrong.m`](fixtures/provider-modules/wrong.m) declares `value(): big` and fails
against `Value.value(): int`, even where native storage sizes are equal. Variants
with no used Value members legitimately have empty requirements. No guest code
executes while native admission compares the snapshots. Actual application and
dependency calls continue through the existing dedicated execution path.

This completes native interface admission, not transparent remote `load` or a
general argument/result marshaller. Inferno's link hash is a compatibility check;
it does not describe an arbitrary value graph or synthesize its codec. Full typed
remote binding must connect actual owner execution, value/ref/channel lifetimes
and the existing Unibus/proxy mechanisms. Existing proxy support is not advertised
as support for arbitrary `.m` signatures. Network contract exchange remains
unassigned; all channel control/data still follow the existing S-record layering.

The final native SDK has 740 tests. It and five affected dedicated runtime cases
pass normally and with ASan/UBSan, including all 50 real module scenarios.
LeakSanitizer is disabled because the execution environment cannot support it;
explicit allocation, provider and image-buffer lifetime checks remain enabled.
Imported Inferno sources remain byte-identical to their pinned revisions.

## Contract after take and during load / Контракт после передачи окружения

`dvm_launch_request_take` now clones the accepted immutable requirements into
`handoff.contract` before committing READY → TAKEN. Clone OOM leaves READY and
all original owners intact. Request requirements remain immutable and available
for review even after a successful take; moving their pointer would break that
lifetime. Handoff free retires only its independent copy.

The pre-publication process constructor copies the contract into its immutable
`DVM_LaunchContext`. The typed marker must be 0/1 and exactly agree with contract
presence. Missing requirements or an undeclared contract refuse construction.
Snapshot OOM leaves the caller's handoff intact. Parent and children share this
native context through its existing atomic references; retiring the parent cannot
remove the child's required interface. The final context release frees its copy.
No guest pointers, callbacks, wait loops or new service threads are introduced.

[`dvm_launch_module_checked`](module_interface/launch.h) receives the actual owned
Module returned by the existing reader. On an ACTIVE owner, an exact match between
the caller's load name and the inherited launch label triggers another comparison
against the actual parsed exports. Provider-backed, native-file and builtin return
paths use this hook. A mismatch or failure to allocate export metadata unloads the
input Module, sets `launch target interface changed or cannot be checked` and
returns NULL. It does not retry another side or query a provider a second time.
On success, the same Module proceeds to ordinary Inferno linking.

This extra hook applies to exact named-target loads. It is not an alias identity
resolver: a different load name, dependency or opaque owner label is not silently
interpreted as the same target. Those paths retain ordinary Inferno type checking.
A general remote module binding must explicitly carry the checked target and
contract into its marshalling/invocation path; this hook does not complete that
work. Matching signatures permit compatible live updates and do not freeze files,
module versions, namespace stacks or provider data.

The new native context test checks independent request/handoff/context ownership,
parent-before-child retirement, both context-copy allocation failures and invalid
typed markers. Typed take allocation refusal now includes its extra contract clone.
The ten additional interpreter/JIT runs cover provider/client target replacement
after admission, both real export-snapshot calloc failure positions, and a child's
load after its parent explicitly switches the provider's future target bytes.
They observe guard refusal before ordinary linking rather than treating any nil
load as proof. A private control omitting this loaded-target guard fails with the
intended semantic diagnostic in both engines, even though Inferno's later linker
also rejects the incompatible implementation. Compile errors, crashes and timeouts
are not counted as successful detection.

## Explicit object binding / Явная привязка объекта

[`binding.h`](module_interface/binding.h) now offers a name-independent native path:

| API | Ownership and result |
| --- | --- |
| `dvm_module_check_contract(module, required)` | Consume owned local Module; return the same owner/NULL after the complete contract check |
| `dvm_module_link_contract(module, imports, mkmp, required)` | Consume owned local Module on every outcome; return the original linker's owned Modlink/H |

The caller supplies the actual already loaded implementation and explicitly chosen
requirements. No path string, current launch label or namespace search determines
which object gets checked. NULL requirements refuse; an allocation failure must
not silently choose untyped linking. Invalid module/import-table pointers of the
ordinary NULL/H forms also refuse. Other pointers must refer to valid local parsed
metadata; this API does not validate arbitrary native memory supplied by a peer.

The complete required interface is checked before `linkmod` is entered, including
members unused by the current import table. The standard Inferno linker then
independently verifies actual requested imports and performs its ordinary MP/frame
construction. `mkmp` keeps its original meaning: nonzero creates independent module
data. A refused precheck unloads its input once, without MP construction or another
source attempt. Native snapshot allocations are retired before any managed linker
allocation or exception. Linker failures/exceptions retain the original cleanup
contract. The named-load hook delegates its matching step to this same checker.

```c
/* Owner/controller has one owned reference to the actual selected local Module.
 * imports stays protected by its caller Module; required is a live native snapshot. */
Modlink *bound = dvm_module_link_contract(candidate, imports, 1, required);
candidate = NULL; /* Consumed on success, refusal or a linker exception. */
/* On success, publish bound into an appropriate managed root before further
 * managed allocation/wait. The caller eventually consumes its owned Modlink. */
```

Only the ACTIVE local owner/controller may use this bridge. Module, Type, Import,
frame and code pointers remain inside that runtime; none may be sent to another
native handler, VM or transport as an implementation of remote binding. A held
Module, followed by its Modlink, retains the actual chosen implementation through
ordinary native ownership. A later VFS bind does not cause this API to look up a
new object. This does not turn a filename or a system ID into execution authority.

The 50 real interpreter/JIT module scenarios also exercise explicit bindings of
source-built images parsed under an unrelated alias label. Success preserves the
exact Module identity, original entry frame/PC and requested link count. Refusals
cover real int/big mismatch, a required export unused by the current imports,
missing requirements, NULL/H import tables, invalid modules and each of the two
actual export-snapshot calloc positions. A subsequent valid bind must still work.
Observers count actual ordinary linker entry and direct input unload; they do not
replace a Module, linker result, MP constructor or guest execution. Observation
state is local to the startup controller's native thread.

A private control that skips complete requirements for the unused-member case
fails semantically in both engines, where ordinary import linking alone would
succeed. Build errors, signals and timeouts do not count. Eight affected runtime
checks pass normally and with ASan/UBSan, including module retirement, module
cycles, source-generation idempotence and the 50 provider-module scenarios. Public
binding declarations also compile standalone as strict C89 and C++98.

This is an explicit native owner-side primitive. Ordinary Limbo `load` still uses
its existing named-load and Inferno-link paths; it has not acquired a general
remote module value or an implicit alias-to-contract association. Future generated
proxies can use the explicit bridge after resolving and loading their actual
owner-local implementation. Remote discovery, marshalling and invocation remain
separate work; no wire layout or S-record tag is assigned by this API.
