# Code, calls and data / Код, вызовы и данные

Typed module implementation lookup is **caller-local first**. A client considers
its side of the visible namespace before the provider side; the provider considers
its own side before the reverse side. The same `.m` interface can describe either
implementation. Choosing a source is separate from routing execution to its owner.

Вызов через `.m` симметричен: сначала своя сторона вызывающего, затем обратная.
Это порядок разрешения реализации typed module, а не смена места выполнения:
выбранный модуль должен исполняться у своего владельца, со своим подготовленным
окружением, зависимостями и устройствами. Сформированная VFS может включать узлы
других машин; lookup не вводит запрет на такие вызовы.

## Caller-relative selection / Выбор относительно caller

[`dvm_resource_plan_call`](vfs/resource.h) takes the caller's namespace, logical
module name, trusted job-relative local role and empty output plan. It resolves
under one namespace lock, retains the selected candidates and performs no provider
I/O. Close successful plans with `dvm_resource_close` on their source owner.

| Caller-local role | Candidate order at one effective union target |
| --- | --- |
| `DVM_ORIGIN_TERMINAL` | Terminal-class candidates, then remaining candidates |
| `DVM_ORIGIN_PROVIDER` | Provider-class candidates, then remaining candidates |

Order inside each class remains the existing stack order. Unclassified branches
retain their relative order in the remaining class; no physical location or
permission is inferred from addresses, file suffixes, nicknames or node types.
The longest explicit bind target still wins. An exact file binding cannot fall
through to an unrelated ambient local root merely because it is on the other side.
Retained import-directory anchors keep their existing relative-name contract.

The operation does not rewrite the namespace, its revision, ordinary data plans,
other callers' selections or previously retained plans. Opposite caller roles may
therefore use the same namespace concurrently, and independent namespaces may
have different visible stacks. An ordinary bind now preserves each copied
source's origin association as well as its actual provider/anchor reference.
Directory unions, exact-file aliases and independently moved child stacks keep
that association; a new visible name does not move the underlying module.

Only real absence permits the launch namespace's normal fallback. A permission
or provider error is final for that selection; it does not probe an opposite-side
implementation. Existing explicit `MAUTORWX` policy and effective-access handling
remain separate. Lookup precedence creates no read/execute permission.

## Runtime ownership / Владение runtime

The actual Dis loader uses caller-relative selection before cache lookup, opening
or parsing. Each permanent native process owns an immutable `module_local_role`,
independent of `provider_execution` image-admission state. Ordinary startup chooses
the terminal class. Children inherit the parent's local role; native environment
adoption establishes the host-selected role before the first guest instruction.

`DVM_LaunchHandoff.local_role` is a native-only constructor setting. Zero retains
the existing provider-startup default; explicit TERMINAL/PROVIDER chooses the
corresponding local class. Unknown values refuse adoption without consuming the
handoff. This field is neither serialized nor accepted as a peer's claim of
permission or execution location. Trusted host code supplies it only after choosing
the runtime and granting the two roots. SDK consumers must rebuild for the native
handoff layout change and keep handoff construction zero-initialized.

The bootstrap controller has no current guest yet and keeps its existing initial
module selection convention. Builtins beginning `$` retain their global registry
contract. Existing module instances/handles stay bound to their selected code;
a later bind or new lookup does not silently retarget an already obtained instance.

## Explicit asset intents / Явное назначение ресурсов

[`dvm_resource_plan_intent`](vfs/intent.h) remains the separate source-acquisition
API for the existing provider-first code-asset policy. It does not receive the
caller-relative module-binding role.

| Intent | Existing acquisition policy |
| --- | --- |
| `DVM_RESOURCE_DATA` | Current cwd and ordinary stack order, initially terminal-first |
| `DVM_RESOURCE_DIS` | Explicit provider-first Dis image acquisition |
| `DVM_RESOURCE_INTERFACE` | Explicit provider-first interface source acquisition, including `.m` bytes |
| `DVM_RESOURCE_NATIVE` | Reserved provider-first native image acquisition; no format/decoder assigned |
| `DVM_RESOURCE_SWF` | Provider-first initial/nested SWF acquisition |

Ordinary typed `load` now uses `dvm_resource_plan_call`, rather than conflating
that call's source selection with fixed provider-first asset acquisition. A `.m`
typed interface and reading a `.m` source file as an asset are distinct operations.
`Sys.open("payload.dis", ...)` still performs ordinary data lookup; suffixes never
choose a policy. FlashVM movie acquisition retains its existing explicit intent.
These APIs use native identifiers, not newly assigned S-record tags or wire flags.

## Actual verification / Реальная проверка

The native call-order test constructs five visible branches: two terminal-class,
two provider-class and one unclassified. It checks exact stable ordering for both
caller roles and unchanged ordinary-data ordering. Two simultaneous native callers
perform512 actual selected-source reads on the same namespace without changing its
revision or another caller's frozen selection.

It also checks full-directory union bind, exact-file bind confinement, copied
namespace independence and moving one file's provider layer without changing its
parent/sibling. Both caller roles reach the reverse side on absence and refuse
permission errors without querying the opposite side. Invalid roles/paths clear
empty output without retained owners. Every provider must retire exactly once.
The earlier provider-first asset-intent test remains a separate regression.

The actual provider-module fixture runs eighteen variants in interpreter and JIT,
including accepted client-local and provider-local parent/child typed application
and dependency calls. Each loader selection is observed on its actual ACTIVE
caller. The selected runtime reads its granted test images and executes real
native-cell code; no fake Module/call result substitutes for execution. Malformed
native handoff local-role metadata is rejected before adoption in launch-context
acceptance. Native allocation/image/FD retirement checks remain enabled.

This stage establishes caller-relative selection. Transparent cross-machine
`load`/typed-call routing and general marshalling remain unfinished. The internal
[provider image reader](PROVIDER-MODULES.md) is not a substitute for routing a
remote callee to its owning runtime. No guest pause, cooperative service loop,
foreign heap pointer or source-role access restriction is introduced here.

The full native SDK passes730/730 tests ordinarily and with ASan/UBSan. Seven
selected runtime regressions pass in both builds; final focused checks rerun the
36 actual module executions and launch-context rejection/ownership scenarios.
Private source omissions forcing provider-only preference or dropping bind origin
copy fail with the intended semantic messages, not compile errors or crashes.
LeakSanitizer remains disabled in this restricted environment; native ownership
and image-buffer retirement observations are kept enabled.

[Native launch routing](LAUNCH-ROUTE.md) now carries this selected source into an
immutable owner address and bounded admission queue. Automatic network discovery
and transparent typed module-call marshalling remain unfinished.

[Provider-owned discovery](PROVIDER-LAUNCH.md) now obtains native owner addresses
from the selected provider, preserving filtered mappings and live export policy.
It adds no remote address wire exchange or general typed-call marshalling.
