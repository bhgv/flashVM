# Provider-backed Dis loading / Загрузка Dis из VFS провайдера

The running execution-provider instance can now load ordinary native-cell `.dis`
applications and dependencies from its granted VFS providers. The existing typed
Limbo `load`/module-call interface and original Dis parser remain unchanged.
Code lookup uses the prepared namespace's **provider-first** policy; ordinary data
keeps its terminal-first policy. No display context or application-specific remote
client is required for this loader.

Исполняющий DisVM на стороне провайдера читает `.dis` через выданные VFS-handles.
Зависимость, загруженная обычным `load`, использует то же окружение и исполняется
в том же runtime провайдера. Файлы данных сохраняют прежний terminal-first порядок.
Это законченный loader-этап, а не сетевой протокол запуска или remote-function proxy.

## Execution placement / Место исполнения

A trusted native launch handoff establishes `provider_execution` before process
publication. Children inherit that immutable execution role along with their
ordinary namespace ownership. A running guest cannot enable it. A plain local
terminal context still refuses provider-backed code loading: mounting a remote
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

Sixteen cases cover normal loading, denied opens, early EOF, oversized low/high
lengths, failed opened metadata, invalid Dis bytes, cancellation after a copied
prefix, image-allocation failure, malformed read acknowledgement, missing handle
cancellation capabilities, accepted child killwait during an actually blocked
provider read, and refusal outside the native execution role, noncanonical split-length parts and an opened
directory replacing earlier file metadata. Each runs in both
modes. Native allocation/free observation verifies real image-buffer retirement;
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
