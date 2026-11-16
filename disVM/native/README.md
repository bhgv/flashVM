# Native Inferno libraries / Библиотеки Inferno на C

Copyright (c) 2026 bhgv

This package imports **all of lib9 and tools/libstyx**, together with the required non-graphical headers,
legal notices and native Linux64 support, from Inferno's `experiment` branch.
[UPSTREAM.json](UPSTREAM.json) records the exact revision and every source hash.
The source files in `src/` are unchanged; fixes are made and tested in the owning
Inferno repository before vendor-sync imports them.

Пакет содержит **полные lib9 и tools/libstyx**, необходимые неграфические заголовки, лицензии и поддержку
Linux64 из ветки `experiment`. Исходники в `src/` сохраняются побайтно. Выбор
собираемых файлов берётся из оригинальных `lib9/mkfile` и `mkfile-Posix`: варианты
других ОС тоже сохранены, но одновременно с POSIX не компилируются.

## Build and use / Сборка и использование

```sh
cmake -S disVM/native -B build/inferno-native \
  -DINFERNO_HOST_SUPPORT="$PWD/tools/limbo-bootstrap" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/inferno-native --parallel 4
ctest --test-dir build/inferno-native --output-on-failure
python tools/vendor-sync.py disVM/native
```

CMake targets `inferno9` and `infernostyx` produce `lib9.a` and `libstyx.a`.
Link server consumers with `infernostyx`; it carries lib9, pthread and math
requirements. Use the supplied target include directories, including the actual
native architecture's `lib9.h`, rather than the historical bootstrap ARM header.
`INFERNO_SOURCE` can select an owning upstream checkout for testing fixes before
importing them. No installed Inferno SDK or deployed Python runtime is needed.

Цели `inferno9` и `infernostyx` собирают исходные библиотеки, а не переписанные
кодеки. Для проверки upstream-исправлений можно явно задать `INFERNO_SOURCE`.
Python нужен только при конфигурации. Пакет сейчас рассчитан на Linux64;
на этой машине проверен amd64. Выбор arm64/riscv64 предусмотрен, но выполнение
на этих архитектурах этим набором результатов не подтверждено.

The native PC and hardware FP helpers already present in `experiment` are used.
The proven bootstrap host preparation scripts supply thread-local error state,
reentrant formatter/decimal support and native synchronization. These adapters
remain outside the exact source snapshot. They do not schedule guest processes.

Используются уже имеющиеся в `experiment` native64-функции PC/FP. Явные адаптеры
обеспечивают локальные для потока ошибки и синхронизацию форматтера/decimal-кода;
они не управляют исполнением гостевых потоков.

## Checked changes / Проверенные исправления

The imported upstream server fixes preserve 32-bit fids and 64-bit Qid helper
paths, avoid pointer narrowing in its existing invariant macro, retain a live
fid during same-fid walks and refuse replacement of another live fid. Read/write
callbacks receive a real native `ulong` temporary. Their returned counts are
checked before narrowing; read capacity is bounded by the response buffer, and
open modes are enforced. Partial and zero results are preserved without replay.
The public `styxqid(Path, int)` signature requires rebuilding callers.

Проверки исходного dispatcher используют настоящие C-кодеки и перехватывают
только отправку ответа. Они покрывают коллизии младших 16 бит fid, старшие биты
Qid, same-fid walk, режимы открытия, 64-битные смещения, нулевые/частичные
результаты и ошибочные native-счётчики больше запроса, включая значения выше
32 бит. Отдельно восемь native-потоков проверяют изоляцию ошибок и FP-состояния
и параллельное форматирование. Проверки работают и в release-сборке.

## Remaining integration / Оставшаяся интеграция

The complete libraries compile and are used by codec, original server-dispatch
and concurrent native-state acceptance. This is not yet a production native
VFS listener or `Sys.mount` client. The kernel-bound client in `emu/port/devmnt.c`
needs a separate Chan/Mnt/RPC host adapter. Do not link the new lib9 alongside the
bootstrap's other lib9 archive in one executable: unify those dependencies first.

Полная проверка C-сервера ещё не завершена. Реальный POSIX TCP уже проверен на
синтетических данных. Первый файловый grant-адаптер подключён к DisVM VFS.
Остаются полный сервер публикаций каталогов и native mount-клиент, провайдер проверенных credentials/auth-fid, полная семантика walk
и проверки на других платформах. Подготовленный корень соединения остаётся
живым обычным каталогом с независимыми правами; снимок дерева не нужен.

See [native Styx integration](../runtime/STYX-NATIVE.md) and the
[live connection-root contract](../runtime/STYX-SERVING.md).

Original licenses remain in [src/NOTICE](src/NOTICE) and
[src/lib/legal](src/lib/legal); this import does not relicense Inferno.

## Graphics boundary / Граница графики

All graphics, including FreeType, belongs to FlashVM. Neither this native
package nor its library targets imports Inferno Draw/Memdraw/Tk/Prefab, FreeType,
PNG or SVG implementations/headers. The explicit header whitelist contains only
protocol, scalar runtime and FP declarations required by these libraries; complete
lib9/libstyx source directories remain intact. Floating-point text formatting
(`gfltconv`, `g_fmt`, `dtoa`) is numeric conversion, not a graphics operation.
DisVM application adapters issue rendering requests through FlashVM APIs.

Вся графика, включая FreeType, принадлежит FlashVM. В native-пакете отсутствуют
графические библиотеки и их заголовки. Полные исходники lib9/libstyx сохранены;
сопутствующие заголовки выбираются явно. Числовое форматирование не относится
к графике. Прикладные адаптеры DisVM обращаются к API рендеринга FlashVM.

## Stream/stat acceptance / Проверки потока и stat (runtime0.138)

The original C server retains incomplete frames, separates coalesced requests,
refuses impossible frame lengths and completes short transport writes, retrying
EINTR. Tests exercise every split in a real encoded Tread and queued frames using
a deterministic transport boundary; this is not a real TCP listener test.
Stat uses the bounded reply capacity rather than a 512-byte limit. Wstat has
message-bounded string storage and checks decode failure before invoking callbacks.
A 1200-byte name passes both stat and wstat through the original dispatcher.

Исходный C-сервер сохраняет неполные кадры, разбирает склеенные запросы и досылает
короткие ответы с обработкой EINTR. Проверены все разрезы запроса, очередь кадров,
неверный размер кадра и stat/wstat с именем длиной 1200 байт. Проверки проходят
под ASan/UBSan; это не заменяет испытание настоящего TCP-соединения.

## Session acceptance / Проверки сессии (runtime0.139)

New clients must negotiate Tversion first. Each connection has an independent
full-packet msize, capped at MSGMAX; this implementation refuses values below
256. Read capacity, stat packing and outgoing replies respect that bound.
Incoming oversized frames disconnect the client. A new valid version request
clunks all fids and closes open backend handles before starting the new session.
Dot-suffixed 9P2000 variants negotiate base 9P2000; other versions return
Rversion `unknown` and cannot attach until renegotiated.

Новые соединения начинают с Tversion. Ошибочный tag или слишком маленький msize
не заменяют прежнюю сессию. Отказанный attach не публикует fid и восстанавливает
имя пользователя/attach-name; при живых fid менять эти имена нельзя. Close
выполняется до освобождения backend клиента. Ошибочный кадр больше не зацикливает
styxprocess: клиент освобождается сразу, без висячего current-client указателя.

Tauth and attaches using auth-fids explicitly fail until an authentication
provider exists. Packet names do not authenticate a peer. Attach callbacks own
rollback of any private backend state they change; the library rolls back only
its own identity/fid state. The Client layout changed: rebuild embedding callers.
Callbacks run synchronously on their owning native server thread and must not
recursively dispatch requests or stop guest workers globally. Ordinary native
metadata helpers remain available to construct and update a live served view.

Проверка исходного dispatcher покрывает согласование лимитов, сброс сессии,
read/stat в пределах msize, отказанный attach, запрет смены полномочий при живых
fid и закрытие handles до backend. Ошибочный кадр проверяется через styxprocess
с ограничением времени теста. Это детерминированный транспортный стенд, а не
испытание реального TCP listener или криптографической аутентификации.

Backend newclient rejection is honored before requests are served; freeclient
releases partial initialization. / Отказ backend в newclient отключает клиента
до обработки запросов; freeclient освобождает частичную инициализацию.

Walking through an ordinary file now returns Rerror and releases the temporary
fid instead of terminating the server. / Попытка пройти Twalk через обычный файл
возвращает Rerror и освобождает временный fid, не завершая сервер.

## Live directories and teardown / Живые каталоги и закрытие (runtime0.140)

In needfile=0 provider mode, root directory reads use the live read callback even
when a library root entry exists. Callbacks receive actual 64-bit wire offsets;
replies must contain complete valid stat records. A provider walk denial never
falls back to hidden cached children. Use this mode for authority-filtered live
views, supplying both walk and read callbacks for that view; filtering a listing
alone does not deny guessed paths. Providers enforce access at the final operation.
needfile=1 retains the original library-tree behavior for compatibility.

Внутренний каталог выдаёт только целые stat-записи. Маленький ненулевой буфер
возвращает ошибку без продвижения курсора и ложного EOF. Продолжение требует
байтового смещения предыдущего ответа; offset=0 начинает чтение текущего дерева
заново. Изменения видны без reattach. Дерево не фиксируется: при изменении между
порциями обычны пропуски/повторы; клиент может перечитать каталог с нуля.

POSIX styxend closes client fids before backend release, frees the library tree,
closes client/listener descriptors and frees readiness state. Repeated end and
reinitialization after end are covered, as are announce/socket-init failure paths.
Backend-owned private payloads remain the backend's responsibility. Shutdown
runs on the owning native service thread and must not race dispatch.

Проверка lifecycle использует реальные native fd с подменой только announce.
Отдельный настоящий TCP loopback-тест связывает полные libstyx/lib9 и исходный
POSIX transport: два клиента с разными msize и приватными представлениями проходят
Tversion/attach/walk/open/read/write/stat. Сохранённые fid видят новые имена,
mode и данные; второй клиент остаётся независимым. При завершении проверяются
четыре backend-close и два freeclient. Имена пользователей здесь маршрутизируют
синтетические представления и не являются аутентификацией.

Public styxend is implemented by each transport port; its signature is unchanged.
The new private styxendserver helper owns common cleanup. Nt has the corresponding
teardown code but no Windows acceptance here. The retained Plan9 helper-process
port returns an explicit unsupported shutdown error instead of false success:
shared state cannot safely be freed until those helpers have a join protocol.
That process model is not imported into the dedicated-native-thread VM.

Исходники и ограничения: [upstream tests](src/tools/libstyx/tests/README.directories-lifecycle.md).
Все восемь native-проверок, включая TCP, проходят также под ASan/UBSan на amd64.
Это не сертификация других архитектур, готовый mount-клиент или production-служба
VFS с проверкой credentials. Графические библиотеки в этот пакет не добавлены.

## File-grant bridge / Адаптер файлового grant (runtime0.141)

The optional [VFS file-capability bridge](vfs/README.md) consumes prepared VFS
provider capabilities through a scalar C boundary. It links the existing native
publication/provider/handle implementations and non-graphical FlashVM ports,
without a second lib9 archive. An original-C TCP fixture uses actual publication
tickets and proves independent equal-Qid opens, precise partial acknowledgements,
read-only ceilings, uncertainty refusal and open lifetime after unpublishing.

Расширение C API libstyx: `styxgetaux`, `styxsetaux` и consuming callback
`Styxops.freefid` дают отдельный native-контекст каждому fid. Контекст заимствуется
только во время dispatch и освобождается до backend клиента. Нельзя заменять
неосвобождённый контекст; клонирование unopened fid не копирует handle. Это
embedding API, не расширение сообщений 9P; Client/Styxops требуют пересборки.

This is the first file-only bridge, not a general directory publication server,
authenticated network service or mount client. Runtime0.143 adds per-connection native preparation and pending owner review
through [PEER.md](vfs/PEER.md). Runtime0.144 adds deferred Rattach; verified remote credential
routing remains separate work. The test's local grant is explicitly
approved by its native owner before serving synthetic data. See the bridge guide
for explicit build inputs and API ownership/offset limits.

With the explicit VFS option enabled, 12 native tests pass locally and under
ASan/UBSan on amd64. / С явной опцией VFS локально проходят 12 native-проверок,
также под ASan/UBSan на amd64.

## Directory grants / Каталоговые grants (runtime0.142)

The [directory/node bridge](vfs/TREE.md) extends the file bridge without importing
a namespace tree. It walks one component inside a retained prepared provider,
publishes one pending directory entry at a time and commits only complete encoded
records. Offset zero reopens a checked cursor through the same provider capability;
a changed object identity is refused. Pending records recheck visibility/metadata.

Исправлены metadata обычных удерживаемых view-handles: они запрашиваются у
открытого объекта, затем маскируются текущей политикой и виртуальным именем.
Синтетические/составные виртуальные родители сохраняют прежнюю view-семантику.
Байтовые endpoints без handle-stat продолжают работать; для них нельзя выдавать
factory/path metadata за stat уже открытого объекта.

Actual TCP acceptance now includes a local directory publication grant, hidden
nodes and live revocation, whole-record pagination, invalid offsets, nested walks,
root-clamped parent traversal, newly added entries, rejected escaping components,
held opens after unpublish and refusal to rebind a replaced directory. Native
checks include malformed provider names and captured metadata after replacement.
The optional suite has 13 tests; authenticated remote attach/mount, complete walk
conformance and general production service admission remain separate work.

## Connection roots / Корни соединений (runtime0.143)

[Native admission API and example](vfs/PEER.md) bind a publication request to trusted
host provenance and a fixed rights ceiling. Submit-only endpoints safely outlive
joined publication close; granted providers and opens retain ordinary lifetimes.
TCP acceptance uses different prepared roots for equal wire usernames/Qids.
Deferred native review and Rattach are supported; see runtime0.144 below.

## Deferred attach / Отложенный attach (runtime0.144)

[Native deferred Rattach](vfs/DEFERRED.md) now preserves pending tag/fid until a
reviewer signals the socket owner. Flush/reset/disconnect/shutdown retire the
request before any later reply; no client retry or guest suspension is required.
The original Inferno implementation and VFS TCP tests cover dedicated native
review, pipe wakeup, cancellation, denied publication close and shutdown.

## Native mounted client / Native mount-клиент (runtime0.145)

[Native client/provider API and example](vfs/MOUNT.md) use original Inferno lib9 codecs,
independent remote fids, precise chunks and ordinary provider lifetime. Real TCP
tests cover prepared publication, partial writes, lost acknowledgements, bounded
framing/deadlines, directory records and native loss notifications. The first
client serializes one RPC per connection; multiplexer/Tflush, authenticated
transport, stable subroot bind and guest Sys builtin wiring are separate work.

## Native multiplex/flush / Native multiplex/flush (runtime0.146)

[Connected-fd client API](client/MULTIPLEX.md) adds a dedicated native stream owner, bounded
outstanding RPCs, unique/paired flush tags and cancellation/deadline fences.
Original acknowledgements survive cancellation races and flush failure. Final
owner drop does not join; explicit native shutdown can await fd retirement.
The same VFS provider works with this client; no guest execution/pause is used.


## Native terminal events / Native terminal events (runtime0.147)

[Automatic connection notifications](client/EVENTS.md) connect idle fd-owner EOF
and terminal failures to each mounted provider journal. Native subscriptions own
journal references independently; unmount does not wait for callbacks. Shared
clients retain independent mount subscriptions. Explicit shutdown differs from
network failure, and normal final release is quiet. The suite now has69 tests;
heartbeat and provider-side content/rights/load reports remain separate extensions.


## Native heartbeat / Native heartbeat (runtime0.148)

[Standard Styx heartbeat policy](client/HEARTBEAT.md) optionally detects silent connections
using a reserved no-fid Tflush/Rflush exchange. Native state snapshots distinguish
send stalls, missed replies, recovery and terminal policy timeout. Each mounted
provider observes transitions automatically through its existing journal, without
guest execution or wire-format extensions. An admitted probe retains its tags and
captured deadlines through disable/reconfiguration. Pending reads remain independent.


## Deferred byte reads / Отложенные byte reads (runtime0.149)

[Original C libstyx deferred read API](vfs/READ.md) retains ordinary Tread until a
native producer wakes its socket owner. Other clients and heartbeat requests
continue; flush/reset/disconnect/end safely retire pending requests. One-record
TCP fixtures preserve queued data on short buffers. Actual native client tests
combine pending reads, automatic heartbeat and explicit cancellation. Wire Styx
is unchanged; the public provider event format remains undecided.


## Provider event resource / Ресурс событий провайдера (runtime0.150)

[Native event-resource API and examples](vfs/EVENTS.md) expose a journal as an ordinary
readonly VFS file with explicit native access filtering and host-selected encoding.
One read returns one record; readiness integrates with deferred Styx reads without
guest execution. Cached heads survive short buffers and recheck access at delivery.
Queues are bounded, overflow broadens invalidation, and finish drains before EOF.
No default public wire format/global path is installed; remote client decoding
and subscription discovery remain separate integration work.


## Remote native event feed / Удалённая native-подписка (runtime0.151)

[Configured remote event feeds](vfs/FEED.md) attach a host-selected endpoint/decoder
to an ordinary mount. A native worker holds a cancellable long-lived read and
publishes into the mount journal without guest progress. Health and remote source
generations are independently rebased with atomic local append. Watch admission
keeps ordinary RPC capacity; provider destruction stops without waiting for Rflush
or decoder return. Discovery and a standard record codec remain separate work.


## Live mount event control / Управление подпиской mount (runtime0.152)

[Versioned native mount controls](vfs/CONTROL.md) replace/detach a live mount event feed
without changing ordinary fids. Preparation precedes CAS commit and adopts decoder
state only on success. Clear/retirement can invalidate an admitted preparation.
Native predecessor fences preserve two-slot watch capacity through delayed Rflush
and rapid replacements. Cutover posts conservative invalidation; no guest progress,
pause, transport reconnect or event-codec discovery is required.


## Independent native subscriptions / Независимые native-подписки (runtime0.153)

[Bounded subscription factories](vfs/SUBSCRIPTIONS.md) create separate event queues, caches
and readiness descriptors for independent subscribers under one explicit host policy.
Quota includes setup and retiring callbacks; close/free never joins a publisher.
Original C Styx TCP fixtures verify per-fid readiness, independent short/flush/reopen
behavior and two real native mounts receiving one publication independently.
Shared-inbox APIs remain available; wire encoding and endpoint discovery stay explicit.


## Single-file publication and leaf bind / Публикация файла и точечный bind (runtime0.154)

[Single-file mount/bind and examples](vfs/SINGLE-FILE.md) support true file Styx attach roots,
direct/prepared publication and explicit missing leaves in populated directories.
Sys.bind, Vfsexport.mount and Vfsctl share file-stack ordering; directory reads
show graft names beside existing siblings. No host placeholders, tree freeze or
wire-format extension is required. Original Inferno changes are pinned byte-for-byte.


## Captured live roots / Захват живых корней (runtime0.155)

[Directory/file capture APIs and examples](vfs/CAPTURE.md) anchor a selected remote
fid for Sys.bind, Vfsctl and Vfsexport.capture. Original-path replacement does
not redirect the bound root; directory membership, policy and rights stay live.
Captured origins retain source feeds and forward invalidations automatically.
Native cleanup never waits on the guest thread. No Styx wire extension is added.


## Native control cancellation / Отмена служебных RPC (runtime0.156)

[Native walk/open/stat cancellation](client/CONTROL-CANCEL.md) adds explicit cancel tokens
with original-reply/Tflush fences. A long walk uses one logical token across
fragments; abandoned fid cleanup never delays its cancelled caller. Unknown
sent Topen state forbids open/data replay on that fid. Tstat output remains
untouched on failure. This is native client API integration, not guest kill wiring.


## Provider data cancellation / Отмена provider I/O (runtime0.157)

[FD-to-Styx data cancellation](vfs/DATA-CANCEL.md) keeps an opt-in native operation
registered through admitted read/write, including prepared views and zero writes.
Guest cancelwait/killwait reaches Tflush without guest callbacks or debug pauses.
Acknowledged prefixes survive; readn never starts a fresh chunk after cancellation.
Legacy providers retain their boundary. SDK handle consumers must rebuild.
Guest open/directory/control cancellation remains separate integration.


## Cancellable provider open/metadata / Отмена open/metadata (runtime0.158)

[Control-operation scopes and API](vfs/CONTROL-WAITS.md) extend guest cancelwait/killwait to
opt-in path/open, held-fid fstat and seek. Native Styx walk/stat/open/clunk chains
use fresh tokens under one source scope. Acknowledgements survive; cancelled
union/MAUTORWX lookup never probes lower providers. Callback tables and guest
Sys ABI stay unchanged. Directory enumeration/bind/capture remain separate.

[Directory cancellation](vfs/DIRECTORY-CANCEL.md) adds native LIST capability,
original Tread/Tflush cancellation and checked prepared-view prefixes. Raw
acknowledged records survive; interrupted views stop probing and invalidate their
cursor. Dynamic namespace guest-dirread admission remains separate. The native
suite has239 cases, including27 directory cases and a union negative control.

[Guest directory waits](vfs/GUEST-DIRECTORY-WAITS.md), runtime0.160, add a native
factory LIST-chain promise and admitted guest dirread cancellation. Dynamic binds
remain live; unsupported late routes fail before callbacks. Cancellable Tclunk
covers intermediate enumeration cleanup, while guest close integration remains
separate. Native acceptance now has248 cases plus real interpreter/JIT guest
coverage for cancellation, managed publication and late-bind rejection.
