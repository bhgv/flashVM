# Detached close cancellation / Отмена отделённого close (runtime0.162)

Inferno's [Sys interface](../../upstream/module/sys.m) has no separate `close`
call. A guest releases `ref Sys->FD`; the last reference eventually retires the
resource. DisVM already detaches its descriptor slot and gate, then transfers
exclusive native ownership to a preallocated close job. That transfer does not
wait for backend I/O. This release keeps that behaviour and adds shutdown
cancellation to eligible detached native closes. No new guest close syscall,
per-guest close wait, cooperative event loop or debugger stop is introduced.

В Sys нет отдельного close: ресурс освобождается при исчезновении последней
ссылки на FD. Слот descriptor и gate отделяются до фонового native I/O. Новый
механизм отменяет поддерживаемый cleanup при завершении close-службы; гостю не
нужно ждать RPC и обслуживать цикл сообщений. Прежнее описание «отмены гостевого
close» следует понимать как этот отдельный native lifecycle, а не новый syscall.

## Native capability / Native-контракт

`DVM_VFS_CANCEL_CLOSE` (16) promises that consuming close honours an active native
source, including a source requested before entry. It must always consume local
ownership exactly once, even after cancellation. It must not restore a descriptor
slot or leave a partially owned handle in caller storage. Other cancellation
flags retain their meanings; `DVM_VFS_CANCEL_MASK` is now31.

`DVM_VfsHandle` appends optional `close_ready(state)`. This is a nonblocking,
allocation-free, native-only capability query under **exclusive close ownership**.
It may inspect currently held substreams, but must not perform provider I/O,
resolve paths, retain arguments or enter the guest/managed heap. The helper
`dvm_vfs_close_cancellable(handle)` returns1 for an empty handle or static CLOSE,
otherwise delegates to this query or returns0. Static CLOSE is appropriate only
when the guarantee holds for the entire handle lifetime.

Provider, prepared-view, namespace-directory and union wrappers delegate the
query to their currently owned inner/header/current streams. No future branch is
opened to answer it. An unscoped union read may retain a legacy branch after the
FD was opened: its close query must then return0, even when the original header
was capable. A not-yet-opened legacy route does not control close. The query runs
on the native closer after all borrowers retire, never inside a GC finalizer.

The handle layout grows; rebuild native SDK consumers. The DVM_VfsOps callback
table, guest Sys ABI, Limbo modules and Styx wire format are unchanged.

CLOSE означает поддержку отмены consuming callback, а не дополнительные права.
Для динамических wrappers проверяются фактически открытые потоки в момент
закрытия. Новых stat/walk/open запросов при проверке нет. Новый указатель query
добавлен в конец native handle; внешние SDK consumers нужно пересобрать.

## Service lifecycle / Lifecycle close-службы

The closer allocates one native source at start, before producers can publish
jobs. Allocation/thread refusal releases partial state and leaves the service
restartable. No additional allocation or callback is added to finalizer queue
publication. Existing preallocated per-open jobs are retained.

For an eligible job the native worker enters a source scope, performs consuming
close and clears that scope before moving to another job. Legacy jobs receive no
new scope. The worker owns no guest PID, Prog, REG, managed roots or FD-table
borrowers. Its source and signal callbacks retain only native cancellation or
independent wake state.

After producers/collectors stop, service stop marks stopping under the queue
mutex, unlocks it, requests the source and joins the worker. Signal callbacks run
outside the queue mutex. An admitted close can interrupt its native wait; later
eligible jobs observe sticky pre-cancel and consume ownership without starting
uncancellable provider I/O. Legacy closes keep their original behaviour and can
still delay drain. No native thread is forcibly killed or abandoned.

The source remains owned until join and is released afterwards. A later start
allocates a fresh source; shutdown cancellation cannot leak into the new service,
a legacy callback or an unrelated native caller. Empty/inactive service handling
and ordinary inline native cleanup keep their prior ownership rules.

Stop сначала посылает native отмену вне mutex очереди, затем ждёт дренирования.
Очередь сохраняет FIFO и consuming close каждого ресурса выполняется один раз.
Legacy callback без обещания отмены не получает ложных гарантий и может ждать
настоящий native close. Принудительного убийства OS threads нет.

## Styx ownership / Владение Styx-ресурсами

Opened fd-owner Styx handles advertise CLOSE. The existing scoped Tclunk adapter
uses Tflush when necessary. An acknowledged Rclunk remains authoritative; an
unacknowledged/cancelled attempt can schedule independent best-effort retirement
of the old fid number. Local close is still consumed once. This does not claim
that a peer acknowledged cleanup when no Rclunk was received. Fid numbers are
never reused, so delayed cleanup cannot close a later remote object.

Independent cleanup uses ordinary mux capacity and deadlines. The native source
belongs to the close service, not the shared connection or a guest operation.
An unrelated caller on that connection receives no inherited scope, but ordinary
mux backpressure still applies. No data/open replay, authentication extension,
graphics dependency or imported Inferno source change is added.

## Verification / Проверки

Two native service tests cover active cancellation and cancellation before the
first subscription, queued pre-cancel, dynamic query, legacy TLS isolation,
source/thread construction refusal, exact FIFO consumption, restart and repeated
stop. Four real original-codec TCP cases run actual runtime close FIFO through
file, prepared-directory and prepared-file Tclunk with success, cancellation,
reply/flush race and lost flush. An independent native call verifies source
isolation with mux capacity left for concurrent best-effort cleanup.

Native union tests also retain an actual legacy current cursor after unscoped
read and verify that close eligibility changes. The actual interpreter/JIT guest
fixture releases64 FD owners, half files and half namespace directories, while
the first native close remains blocked. It proves descriptor slots are reused,
all64 jobs are published before shutdown, payloads are preserved, and all64
resources drain exactly once when the service stops. It runs ten repetitions;
the TCP cancellation case runs twenty. A private control suppressing close scope
admission fails in the real native consuming callback.

The complete native suite now has267 tests and runs normally and under ASan/UBSan
(`detect_leaks=0`). Linux amd64 runtime validation also includes host-unit and
native VFS/thread/close-backlog regressions in dynamic/static builds. Legacy
provider/OS close interruption and other unregistered VFS mutation operations
are not claimed by this native source contract.
