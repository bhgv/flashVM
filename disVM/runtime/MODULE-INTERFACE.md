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
The caller is responsible for pinning valid parsed Import/Module storage while
an adapter reads it. Returned native snapshots own all names/signatures and may
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

All 40 actual interpreter/JIT provider-module scenarios use a real compiler Import
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

The final native SDK has 739 tests. It and five affected dedicated runtime cases
pass normally and with ASan/UBSan, including all 40 real module executions.
LeakSanitizer is disabled because the execution environment cannot support it;
explicit allocation, provider and image-buffer lifetime checks remain enabled.
Imported Inferno sources remain byte-identical to their pinned revisions.
