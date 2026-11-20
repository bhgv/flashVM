# Provider-backed Dis loading / Загрузка Dis из VFS провайдера

The running execution-provider instance can now load ordinary native-cell `.dis`
applications and dependencies from its granted VFS providers. The existing typed
Limbo `load`/module-call interface and original Dis parser remain unchanged.
Typed module lookup uses [caller-local first](CODE-LOOKUP.md); accepted provider-local
launches therefore consider the provider class first. Ordinary data
keeps its explicitly selected policy; [per-instance owner-first environments](VFS-TRAVERSAL.md)
use the actual configured stack for data and loads. No display context or application-specific remote
client is required for this loader.

Исполняющий DisVM на стороне провайдера читает `.dis` через выданные VFS-handles.
Зависимость, загруженная обычным `load`, использует то же окружение и исполняется
в том же runtime провайдера. Файлы данных сохраняют прежний terminal-first порядок.
Это законченный loader-этап, а не сетевой протокол запуска или remote-function proxy.

## Execution placement / Место исполнения

See [Launchcontext](LAUNCH-CONTEXT.md#execution-location--место-исполнения) for the
intended bidirectional/cross-system module-call boundary. A provider process may
call visible modules on the terminal or third systems; those callees must execute
on their owning side. The reader below does not yet implement that routing.

A trusted native launch handoff establishes `provider_execution` before process
publication. Children inherit that immutable execution role along with their
ordinary namespace ownership. A running guest cannot enable it. A plain local
terminal context still refuses ordinary provider-backed code loading: mounting a remote
`.dis` does not implicitly download and execute it on the terminal.

The role is native constructor state, not a wire flag, installation-name check,
credential or proof of physical location. The launch owner must select the actual
execution runtime and authorize its roots first. Builtins and existing native-host
file loading keep their previous behavior. This change does not route an ordinary
terminal-side `load` into a remote RPC, start a network service, transfer arbitrary
FDs, convert typed init parameters or migrate a running module.

Место выполнения выбирает запускающий host: сначала окружение исполнения на
провайдере, затем загрузка приложения в нём. Одного имени удалённого файла для
смены места исполнения недостаточно. Терминал не становится вычислительным
провайдером лишь потому, что видит файл. Role-контекст получают только принятый
native-handoff и его потомки; на wire новое поле не добавляется.

## Opened-object loading / Загрузка открытого объекта

[`vfs/provider_module.c`](vfs/provider_module.c) uses this path:

1. Resolve code intent in the current isolated namespace and retain that selection.
2. Open the selected provider once with ordinary Dis readonly-load semantics.
3. Obtain length/type from **that opened handle**, not a second path lookup.
4. Read exactly the advertised positive length, strictly below8MiB, into one owned
   native image. Short positive prefixes are supported. Early EOF, failure,
   malformed counts, cancellation and allocation refusal reject the whole image.
5. Move the handle to the existing preallocated native closer, finish the operation
   cancellation scope, reenter the guest owner, then invoke the original parser.

Provider permission/open errors are final for the selected file; they never mean
absence and do not retry the terminal/lower provider or native host. Only existing
namespace lookup rules decide genuine missing-name fallback. Read permission
semantics match existing Inferno Dis `load`; code intent chooses source order and
is not a new access grant or an executable-bit reinterpretation.

Opened metadata must describe a finite nondirectory image. Both high and low
length bits are checked before narrowing/allocation. The provider must advertise
native PATH cancellation, and its opened handle must advertise INFO and DATA
cancellation. Unknown capabilities refuse before uninterruptible metadata/read
work. Closing is delegated to the existing native closer so guest termination does
not wait inside provider destruction. All payload buffers and selected-plan pins
are cleaned on success, refusal, parser error and managed reentry exceptions.

Readiness/cancellation belongs to this source operation, with its existing native
wait ledger and sticky cancellation source. The broker/provider continues on its
own native threads. No cooperative guest service loop, foreign guest execution,
forced pause or global execution stop is introduced.

Дерево VFS остаётся живым: каждый `load` получает текущий открытый объект. В память
собирается только ограниченный образ выбранного модуля. Это не рекурсивный снимок
дерева и не обещание транзакционного snapshot содержимого изменяемого файла.
Отказ после частичного чтения не превращает неполный образ в исполняемый код.

## Cache boundary / Граница кэша

Provider images deliberately do **not** reuse the global module cache by logical
pathname. Two namespaces or versions can legitimately expose different bytes at
`/dep.dis` with identical/narrow metadata. The parsed Module still uses ordinary
registry membership/refcounts for lifetime and diagnostics; this is not cache
permission. Native-host cache provenance rules and builtin lookup remain unchanged.

A new load therefore sees updated provider bytes while an already retained module
continues executing its original code. This first implementation trades provider
module cache reuse for source isolation; no provider/session/version cache key or
new wire generation format is invented here.

## Actual acceptance / Реальная проверка

The source-built fixtures use the real dedicated runtime in **interpreter and JIT**
modes. Two independent granted roots publish conflicting code and ordinary data.
The application loads its normal typed dependency from the execution provider,
while ordinary data comes from the terminal. Same-path, same-length and same
metadata updates verify that retained old code remains stable and fresh loads see
the updated version.

Eighteen cases cover normal loading, denied opens, early EOF, oversized low/high
lengths, failed opened metadata, invalid Dis bytes, cancellation after a copied
prefix, image-allocation failure, malformed read acknowledgement, missing handle
cancellation capabilities, accepted child killwait during an actually blocked
provider read, and refusal outside the native execution role, noncanonical split-length parts and an opened
directory replacing earlier file metadata. Each runs in both
modes, including inherited client-local/provider-local calls with their actual application/dependency results. Native allocation/free observation verifies real image-buffer retirement;
provider destructor counters verify that no namespace/handle pins survive shutdown.
No fake Module result or interpretation of source files substitutes for guest
execution.

The [actual Unibus guest test](../native/terminal/order/files/README.md) additionally
loads the application and dependency through original libstyx `Twalk/Topen/Tstat/
Tread` with three-byte file prefixes. Filtered execution and terminal views hide
the raw transport node from the guest. The same guest then exchanges verified
application S-records through its Unibus file grants on that connection.

The new tests live under [`tests/provider_modules`](tests/provider_modules), with
portable Limbo sources under [`fixtures/provider-modules`](fixtures/provider-modules).
`disvm_provider_modules` registers the acceptance; affected native-file cache,
resident-load, load-kill and Unibus launch regressions use only the mandatory native
execution profile. ASan/UBSan acceptance uses a fully instrumented source-built
runtime, with LeakSanitizer disabled in the restricted execution environment.

The admission fixture now uses an independent native handler's
[blocking service wait](LAUNCH-SERVICE-WAIT.md) before prepare/take and controller
startup. This connects native request servicing to the real36 interpreter/JIT
scenarios without introducing a guest cooperation loop or a network launch claim.

The fixture now first routes the selected caller-VFS node through an
[immutable owner address](LAUNCH-ROUTE.md), discovered through the selected
[provider advertisement](PROVIDER-LAUNCH.md) rather than a fixture destination
table. Its client/provider sources have
separate real admission services, and the selected owner supplies its own execution
root before the existing native handler/controller path runs. This remains a
one-host native fixture, not a completed network typed-call implementation.

The retained-resolution extension runs 40 actual interpreter/JIT scenarios.
Address discovery now precedes caller-namespace retirement and later submission.
Two additional cases replace that namespace root with the opposite source before
submit; the selected owner still supplies the parent/child execution environment.
See [deferred resolution](PROVIDER-LAUNCH.md) for ownership and remaining limits.

The same 40 scenarios now also exercise [native interface admission](MODULE-INTERFACE.md):
actual compiler imports are checked against parsed owner exports before root
preparation. A separately compiled int/big result mismatch is refused, while
normal parent/child application and dependency execution remains unchanged.

The first 50 interpreter/JIT scenarios cover the typed-admission boundary. Ten additional runs replace a target
after admission on client/provider sides, refuse each actual snapshot-allocation
position, and verify inherited requirements in a child after the provider changes
future target opens. [Process contract retention](MODULE-INTERFACE.md) and actual
loader-guard refusal are observed independently of the ordinary linker result.

Those 50 scenarios also exercise the [explicit native binding bridge](MODULE-INTERFACE.md):
actual parsed Module ownership, complete requirements before ordinary linkmod,
original entry frames/PCs, unused required members, snapshot allocation failures
and recovery. This is owner-local linking, not transparent network module loading.


There are now **58 interpreter/JIT scenarios**. The eight added runs exercise
provider-side per-Modlink namespace attachment, actual dependency and Sys calls,
FORKNS inside a bound module, separate namespaces for two instances, and inherited
redirected stdin with its original shared offset. Positive explicit binding checks
also exercise `dvm_module_link_prepared`, including final native namespace cleanup.
See [active traversal and instance environments](VFS-TRAVERSAL.md).


The current total is **66 interpreter/JIT scenarios**. Module launch metadata now
belongs to the bound instance as well as its VFS: unbound dependencies inherit it,
and a child spawned from such a dependency retains it without parent frames.
Two instances carry different binary arguments while the containing process
keeps its original payload. See [module-instance launch context](LAUNCH-CONTEXT.md#module-instance-context--контекст-экземпляра-модуля).

A native host can mount an immutable [local adapter](MODULE-ADAPTER.md) and bind
it to the usual load name. This exact provider admits locally generated client
bytecode. A regular provider exposing the same bytes keeps the ordinary
provider execution requirement.
