# Текущий статус многопоточности DisVM/Limbo

Copyright (c) 2026 bhgv

## 2026-11-15: atomic inbox head, allocation-free resync/exit

Runtime0.131 сохраняет непрочитанный head внутри native inbox, а не отдельного FD.
Короткое чтение и close/reopen не теряют и не переставляют событие. Два разных FD
конкурентно читают одну FIFO: успешное копирование и consumption атомарны под inbox
mutex; одна запись возвращается ровно одному reader. Отдельные residents имеют
отдельные очереди. Zero-size read не потребляет событие и не означает EOF.

Overflow теперь даёт один scalar resync без queued duplicate и сохраняет triggering
report при успешной allocation. Repeated loss до чтения coalesces в тот же flag.
Exit также allocation-free: full queue, OOM, repeated retire и короткий буфер не
теряют и не дублируют final notice. Порядок: resync, оставшиеся FIFO records, exit,
затем EOF. Head входит в общий лимит32 nodes; FD больше не содержит staging8KiB.
[Текущий контракт](SYSTEM-EVENTS.md).

Новые native проверки используют настоящий calloc failure injection, close/reopen,
zero/short buffer sentinels, два concurrent reader и full-queue retirement при
отказе allocator. Отдельный native race публикует2000 source reports параллельно200
rebinds и сменам namespace второго inbox. Проверяются final report, hidden scope,
bounded queue, отсутствие source stat/open и final factory cleanup. Реальный guest
делает короткое Sys.read через второй FD и затем читает то же событие через первый.

Dynamic/static host-unit37/37 и guest C/JIT × оба GC observation settings прошли.
Targeted ASan/UBSan native test sources прошли с прежним shared port archive;
leak scan отключён: LSan не работает под ptrace этого окружения. Native lifetime
проверяется также явными final-destruction counters. Private regression control
с published0.130 stream body воспроизводит отказ нового short-read/head теста.
Source-link check:0 stale files. Финальный AMD64 wide77/77.

Способ доставки не меняется: source-driven push, без cooperative handlers, owner
polling, sleep helpers или приостановки получателя. Формат всё ещё experimental;
upstream snapshot, Dis ISA и Sys.m ABI не изменены. Внешний host-filesystem watcher,
Styx receiver/heartbeat и балансировка остаются будущей работой.

## 2026-11-15: source-driven system inboxes, one event per read

Runtime0.130 создаёт независимую native очередь каждого resident. Source journal
непосредственно публикует native сообщения подписанным получателям; изменения
namespace автоматически перестраивают подписки вне mutation lock. Нет опроса на
границах исполнения, cooperative handlers, sleep helpers или пауз получателя.
Чтение только извлекает уже доставленное сообщение. [Контракт](SYSTEM-EVENTS.md).

Production boot монтирует обычный contextual readonly `/dev/sysevents` через VFS.
Bind/hide/unmount/NEWNS работают без literal-path bypass. В очереди нет Prog/heap
указателей; inherited FD сохраняет исходную capability. Epoch отсекает старый scope,
overflow/OOM сигнализирует resync. Отдельный native kind domain не смешивается с
process-control request/reply. Retirement отсоединяет taps без ожидания получателя;
in-flight callbacks удерживают state до завершения и очищаются вне hub mutex.

`Sys.read` и `readn` возвращают ровно одну целую запись. Native DVM_IO_RECORD
останавливает readn без фиктивного EOF. Маленький буфер не выдаёт часть записи;
пустой live inbox возвращает временную недоступность, не idle-message и не EOF.
Внешний формат остаётся экспериментальным, подлежит отдельному согласованию.

Добавлены ordinary ORDWR с shared cursor без create/truncate/append и optional
host-installed diagnostic journal view с независимыми tickets/ack. Pure Limbo
Providerwatch не нужен автоматической доставке и не содержит wait/sleep helper.
[Поведение и границы](PROVIDER-EVENT-FILES.md). FlashVM port version0.1701.

Dynamic/static host-unit37/37. Native test доказывает push до bind owner TLS и до
open device, без source stat/open; проверяет per-thread очереди, scope, overflow,
unmount и retained FD. Два native senders публикуют20000 reports параллельно2000
attach/detach; дополнительно проверен self-unsubscribe и cleanup всех contexts.
Guest проходит C/JIT × оба GC observation settings в dynamic/static: production
device, несколько queued events через readn по одному, NEWNS hiding, ORDWR bytes,
optional ack/race/mask/revocation/reconnect и source lifetime. C89 public headers
и source-link check пройдены. Финальный AMD64 wide77/77.

Автоматическая подписка относится к native providers с journal reports. Styx
receiver/heartbeat, external host-filesystem watcher, balancing и перенос между
открытыми union layers ещё не реализованы. Обычный host bind сам не начинает
генерировать OS filesystem notifications. Upstream snapshot, Dis ISA и Sys.m ABI
не изменены; полного VFS/security/platform acceptance этот шаг не объявляет.

## 2026-11-15: provider changes и independent refresh cursors

Runtime0.129 добавляет retained factory-owned native journal с шестью classes:
RIGHTS, NAMESPACE, CONTENT, CONNECTION, LOAD, NETWORK. Уведомление содержит local
session, source report generation, mask и optional opaque resource ID. Каждый
observer владеет cursor; refresh одного не очищает чужое состояние. Explicit
chunk hints проходят через opened provider wrapper без изменения I/O result и
без diagnostic stat на success. Legacy hints без session остаются diagnostics.
[Контракт и текущие границы](PROVIDER-EVENTS.md).

Selective refresh захватывает точные class versions, вызывает supplied native
reread callback вне journal mutex и подтверждает только ticket. Новое событие
во время query остаётся dirty; failed query cursor не меняет. Epoch replacement
во время query возвращает NOT_FOUND без ack. Journal хранит32 reports плюс class
watermarks; mixed resources, gaps и потеря нужной history расширяют scope до
provider-wide. Duplicate/stale reports и старые epochs не подменяют новое state.
Reconnect API меняет только bookkeeping epoch по expected-session check, не socket
и не opened fid. Generations/sessions не оборачиваются. Native cursor не atomic
shared cache; query staging/version guarantees описаны отдельно.

LOAD/NETWORK пока invalidation classes, не metrics/sampler/балансировка. Сетевой
heartbeat/Styx receiver, background refresh worker, новый guest control module
и переключение открытого file-stack не реализованы. Старые stable-bind identity,
MAUTORWX и module cache правила сохранены; upstream snapshot и Sys ABI неизменны.

Финальный AMD64 wide 76/76; dynamic/static host-unit 35/35. Native unit проверяет
selective/failed queries, copied cursors, callbacks races, generation gaps,
resource zero/mixed scope, overflow, reconnect и4999 reports из отдельного OS
thread параллельно refresh. Новый provider-events guest прошёл C/JIT × оба poll
в dynamic/static: ABC bytes, typed hints, реальные factory rereads, пять refresh
callbacks/шесть state queries, ноль success stat, failed offline query, epoch recovery
и final factory cleanup. Offline/recovery — native simulation, не сетевой acceptance.
C89 syntax check provider/result/change headers пройден; CI новой ревизии отдельно.

## 2026-11-15: precise chunk results и metadata после отказа

Runtime0.128 добавляет optional native read_chunk/write_chunk callbacks и scalar
DVM_IoResult: confirmed prefix count, original status, stop cause, uncertain tail,
независимые diagnostic metadata и change generation hint. Ordinary Sys.read/readn,
write/fprint сохраняют confirmed partial progress при отказе. Readn останавливается
на partial refusal либо explicit final EOF; input tail не меняется. Positive write
с нулевым count — NO_PROGRESS, не EOF. Last outcome доступен пока только native API
под прежним FD pin/FIFO. Никакого автоматического replay или смены открытого layer.

Принятое правило: операция решает доступ, diagnostic info запрашивается только
после refusal/no-progress. Success, short success и EOF не вызывают stat. Wrapper
сохраняет inner probe, исключая двойной запрос. Failed info не затирает I/O count,
status и причину; mode bits не выдаются за effective-access authority. Legacy
failure не сообщает actual count: effects помечаются uncertain, не выдумываются.
[Реализованный контракт и принятый будущий wire contract](CHUNK-IO.md).

Для будущего Styx provider info соответствует Tstat того же fid после отказа,
с deadline/cancellation/connection bounds. Heartbeat changed+generation, session
reset и invalidation оговорены как будущий transport contract. Native result уже
может переносить hint; Styx client/heartbeat receiver, union file cursor/offset modes
и network replay protection здесь не реализованы. Стандартные Styx frames и
upstream snapshot не меняются; MAUTORWX selection при open/create/load сохранён.

Финальный AMD64 wide 75/75; dynamic/static host-unit 34/34. Новый provider-chunk
прошёл C/JIT × оба poll в dynamic/static: actual readn/read/write/fprint counts,
untouched input tails, пять failure probes, ноль success/EOF probes, exact effects
и consuming lifetime. Unit проверяет independent probe failure, malformed count,
legacy uncertainty и один probe через retained wrapper. Public handle/result headers
проходят C89 syntax check. Архитектурная CI этой ревизии учитывается после публикации.

## 2026-11-15: MAUTORWX — явное расширение выбора по access

Runtime0.127 реализует MAUTORWX=8 через constants-only Vfsflags.m. Обычный
Sys.bind сохраняет Inferno flags по умолчанию; при явном MAUTORWX любая помеченная
ветвь включает access-based policy для стопки. Open выбирает первый разрешённый
R/W entry, file module load — R+X до cache, create — первый parent с W+X, без
дополнительного MCREATE. Existing create entry также проверяется на W. Create
может создать верхнюю копию имени, уже существующего ниже; OEXCL collision в
выбранной ветви не вызывает retry. [Контракт, пример и ограничения](AUTO-UNION.md).

FlashVM0.1700 добавляет relative effective access через faccessat/AT_EACCESS от
held anchor, не OR по mode bits разных классов. Optional provider access callback
сообщает актуальные effective rights; без callback права не выдумываются. Новый
open/create/load заново выбирает слой после chmod; живой FD/module остаётся прежним.
I/O выполняется один раз после selection, без retry ошибок/квот/invalid module.
Dynamic mode API, concatenation, guest credentials, полный Chan.walk и Windows
relative permission backend этим шагом не реализуются. Upstream snapshot неизменён.

AMD64 wide 74/74; dynamic/static host-unit 33/33. Native MAUTORWX, classic union
и provider namespace проходят C/JIT × оба poll в обеих сборках. Host chmod/bytes
проверяют owner-vs-other rights, directory write без search, создание верхней
копии, exclusive collisions, cache/execute re-selection и lifetime открытых nodes.
Provider unit проверяет misleading mode=0777, missing callback, live policy,
namespace copy и отсутствие open/эффектов во время selection. Public access/provider
headers проходят C89 syntax check; остальные физические OS не заявляются проверенными.

## 2026-11-15: directory union bind

Runtime0.126 добавляет обычные Sys.bind MREPL/MBEFORE/MAFTER и MCREATE:
ordered native/provider directory layers, retained original-target baseline,
first-existing lookup, создание отсутствующего entry в первой creatable ветви,
копирование union и exact-layer/all unmount. Readonly write не переходит в нижний
слой. Dirread сохраняет duplicate names; lazy cursors следуют живому mount head,
rewind начинает enumeration заново. Новый mount head не подменяет старый FD.
FORKNS сохраняет порядок; opened files и providers переживают unmount.
[Пример и точные границы совместимости RU/EN](UNION-NAMESPACE.md).

Это ещё hosted complete-suffix lookup, не полный retained Chan.walk. Exact union
root chdir явно unsupported; provider chdir/remove/load, guest credentials/rwx
isolation и Sys.mount/Styx остаются отдельными задачами. Vfsctl и nsadmin не
добавлены; write-lower/concatenation не выданы за Inferno semantics. Сверены pinned
cmount/cunmount/createdir и unionread; upstream snapshot остаётся неизменным.

AMD64 wide 73/73; dynamic/static host-unit 32/32. Native/provider union scenarios
прошли C/JIT × оба poll в обеих сборках: chmod handshake, реальные host bytes,
MCREATE targets, readonly отказ, copy/share, replace/remove, duplicate listings,
rewind и retained FD. Provider fixture теперь имеет четыре concurrent worker:
восемь worker плюс новые retained aliases могли превысить 29 ordinary FD до
асинхронного retirement; это исправление oracle, не увеличение runtime лимита.
Signal-debug oracle использует уже проверенный bounded busy retry helper.
CI предшествующей 0.125 завершилась: все шесть jobs успешны, включая TSan и RV64;
проверки новой ревизии учитываются отдельно после публикации.

## 2026-11-15: provider bind и явные расширения среды

Runtime0.125 подключает provider-source Sys.bind(MREPL) к обычной namespace
publication: сохраняется factory и resolved relative prefix, не рекурсивное имя
source alias. Проверяются kinds и совместная revision. File/directory/subtree
и alias-to-alias bindings сохраняются после unmount source; filtered unmount
сверяет factory/relative selection. Никаких новых Sys flags, Dis IDs или opcode.

Новое внутреннее native расширение DVM_PROVIDER_STABLE_BIND разрешает bind только
factory, гарантирующей стабильную relative-name/resource identity. Без promise
bind явно unsupported до полноценного retained node/walk API. Unknown factory
flags отвергаются без поглощения state. Это не stable DLL ABI и не guest право
регистрации native callbacks. [Реализованные расширения и предложение E1](ENVIRONMENT-EXTENSIONS.md)
отделяют текущий host API от пока не реализованного Vfsctl.

Полный финальный AMD64 wide 72/72. Dynamic/static host-unit 31/31;
extended provider-namespace C/JIT × оба poll в обеих сборках проверяет 30 opened states, kind mismatch, file/root/subtree aliases и filtered
alias unmount. Unit отдельно проверяет stale alias, отказ bind без capability,
unknown flags, copy/rebind и final reference cleanup. Provider сохраняет C89
compatibility. Debugger root-scan failure теперь выводит slot/error/kind/extent,
чтобы последующий CI отказ не оставался без диагностики; исходный TSan failure
по-прежнему не объявляется закрытым без нового завершённого CI.

## 2026-11-15: retained-debug busy и предшествующая CI

CI f932e54 завершилась: пять jobs успешны, включая RV64/QEMU; TSan wide 69/70.
Единственный отказ retained-debug сообщил fault array root/payload lost, без
TSan data-race report. Исходный log не сохранял object replies, поэтому причина
самого отказа не объявляется доказанной. Найден дефект oracle: explicit inspector
busy пропускался при root scan. Теперь повторяется только error=busy до bounded
deadline; invalid reads и реальное отсутствие roots не скрываются. Проверены
busy→valid, invalid без retry и permanent busy refusal. Полный retained-debug
со своими omission controls повторно прошёл dynamic/static. Новая TSan CI должна
подтвердить исходный failure mode; до её результата debug gate не объявляется
полностью закрытым.

## 2026-11-15: retained provider mounts и Inferno compatibility boundary

Runtime0.124 подключает trusted host-only provider installation к namespace
resolution, metadata и ordinary open. Namespace/copy/in-flight plan/open handle
удерживают отдельные factory references. Rebind/unmount меняют новые lookups;
старые plans/FD удерживают прежнюю factory до close. Filtered unmount сравнивает
provider identity/relative path, не diagnostic label либо чужой host inode.
Provider chdir/source-bind/remove/load явно unsupported без host fallback;
loader отказывает до native cache lookup. Public Sys.m/guest ABI не изменён.

Совместимость с Inferno обязательна: semantics сопоставлены с pinned
7f2241455734e1142a3254fad9d1ec4f94032ac7 cclose/cmount/cunmount, upstream snapshot
не менялся. Это не полная Chan walk/union/Styx mount или credential policy.
[Таблица совместимости, происхождение и контракт RU/EN](PROVIDER-NAMESPACE.md).

Локально AMD64 wide 72/72; dynamic/static host-unit 31/31. Новый provider-namespace
проверяет восемь guests с FORKNS, 25 opened states, filtered unmount, retained FD,
возврат HOST! decoy и exactly-once factory destroy. Расширенный сценарий прошёл
C/JIT × оба poll в обеих сборках. Native unit отдельно проверяет stale mount,
copy/rebind с одинаковыми labels и last-open lifetime после удаления всех групп.
Provider implementation дополнительно компилируется как C89. Архитектурная CI и
TSan для новой ревизии учитываются только после завершения соответствующих jobs.

## 2026-11-15: provider admission без native type discrimination

Runtime0.123 допускает trusted native handle в зарезервированный descriptor
через consuming publication. Обычные read/write/seek/fstat/fprint/dirread больше
не требуют native file/directory state. List provider обязан иметь abort_list:
ошибка managed publication инвалидирует cursor через native metadata callback.
Invalid table не передаёт владение. Даже zero-byte write теперь удерживает FD
и использует native admission: provider callback может блокировать.

Новый mandatory provider-fd запускает восемь реальных guests на private relink:
24 memory read/write/list states, read/write capability refusals, seek, fstat,
formatted output, zero-write, directory batch/EOF и consuming close. Native
fixtures остаются HOST!, provider bytes независимо дают ABCDE → XBCZ!. Нет
нового runtime CLI bypass: production по-прежнему монтирует только прежние
native resources; guest mount/registration, credentials и полный VFS walk
остаются следующими этапами. [Контракт RU/EN](PROVIDER-HANDLES.md).

Dynamic/static host-unit прошли 30/30; provider-fd прошёл C/JIT × оба poll в
обеих сборках (8 runs). Полный финальный AMD64 wide прошёл 71/71.
Handle implementation сохраняет C89 compatibility.
Архитектурная CI и TSan для этой ревизии подтверждаются отдельно после окончания.

## 2026-11-15: внутренние native provider handles

Runtime0.122 хранит общий owning handle в ordinary descriptor slots. File и
Directory adapters реализуют immutable native operation tables. Read/write,
formatted output, seek, fstat и dirread вызывают provider dispatch; close-worker
получает owning handle через move и закрывает ровно один раз. Поиск handle идёт
до native wait, через прежние rooted FD/FIFO и GC admission. Нового global stop,
managed provider callback, allocation при передаче владения либо guest ABI нет.

[Контракт RU/EN](PROVIDER-HANDLES.md) явно оставляет specialized loader/stdio,
directory-abort recovery и kind admission в прежних adapters. Guest registration,
mount/walk, socket/channel/device providers и credentials ещё не подключены.
Native host rwx и прежние namespace/security limits сохраняются.

Provider unit проверяет независимый callback, unsupported outputs, refused moves,
очистку source и exactly-once close. Host-unit прошёл 30/30 в dynamic и static;
расширенный guest rwx прошёл оба execution/poll static paths. Close-backlog probe
перенесён со старого submit на новую consuming resource boundary: все 1536 guests,
64 pending jobs, files/directories FIFO, три GC profiles и полный drain прошли
в dynamic/static. Первый wide дал 69/70 из-за старой границы probe; финальный
wide после исправления прошёл 70/70. Handle dispatch также компилируется как
C89. Новая архитектурная CI учитывается отдельно. Предыдущая CI v0.1699
ещё выполнялась при начале этого этапа и не подтверждает новый provider layer.

## 2026-11-15: existing write-only open и закрытая прежняя RV64 приёмка

[CI 9d9b0d6](https://github.com/bhgv/flashVM/actions/runs/43668162604)
завершился успешно во всех шести jobs: AMD64 plain/TSan, ARM64, RV64/QEMU,
DIS64 Sys AMD64/ARM64. Это закрывает прежний startup namespace blocker;
physical RV64 hardware acceptance по-прежнему не заявляется.

Sys.open(OWRITE) теперь открывает existing file без создания/усечения и без read
permission, с позиции ноль. Новый port mode wo использует native O_WRONLY;
прежние w/wx/rw не меняют семантику. [Контракт RU/EN](WRITE-OPEN.md).
Guest fixture проверяет actual 0222 opens/read refusal, запись ABCDE → XBCZE,
seek, missing/directory/OTRUNC refusal, bound 0444 refusal и zero live file owners.
Native anchor fixture проверяет direct/relative opens и fdopen failure cleanup.
Локально: полный AMD64 dynamic wide 70/70, static host-unit 29/29;
расширенный guest fixture прошёл static/dynamic × C/JIT × оба poll (8 runs).
Изменённые POSIX порты дополнительно компилируются как C89. Новая
архитектурная CI относятся к этой ревизии отдельно от указанного прежнего CI.
Windows native-path implementation не проверена; Windows anchors unsupported.
Guest credentials, полная provider VFS и sandbox остаются следующими этапами.

## 2026-11-15: проверка host rwx и cross-build anchor gate

Добавлен mandatory workload namespace-permissions: реальные chmod fixtures,
успешные read/create, отказ create поверх readonly файла без изменения bytes,
проверка mode 0444, отказ open/stat без directory x через bind, затем unmount.
Static/dynamic AMD64 проверены в interpreter/JIT × fast/locked poll (8 runs).
Это проверка kernel permissions host user; guest credentials и sandbox не заявлены.
Sys.open OWRITE пока неподдержан и не используется как доказательство permission
refusal. Root host user явно отвергается тестом, а не выдаётся за успешную проверку.

Anchor-test target теперь создаётся и при CMAKE_CROSSCOMPILING. Его наличие
и выполнение проверены cross-конфигурацией AMD64; это проверка CMake gate,
не RV64 execution. Список native wide содержит 70 workloads. Полная новая
архитектурная матрица ещё не выполнена; публикация локальных изменений
заблокирована automatic review до явного разрешения push в master.

## 2026-11-15: RV64/QEMU и единый корень directory anchors

CI на `5e573d2` завершил пять jobs успешно, но RV64 отказал 69/69 workload
до guest startup: `cannot initialize startup namespace`. RV64 job теперь линкует
статические ELF и не задаёт QEMU_LD_PREFIX: sysroot redirection абсолютного `/`
несовместим с последующим host-relative openat от этого directory descriptor.
Отдельный anchor-port gate перед workloads проверяет root-to-cwd lookup,
rename, concurrent opens/removes и cleanup. Runtime lookup не обходит anchors.

Локальный AMD64 static build: 29/29 host-unit; wide 68/69 до исправления
порядка private probe link. Единственный отказ был undefined fesetenv в jit-fp:
probe object стоял после libm. Теперь object идёт перед archives; весь jit-fp
с actual RX faults и omission controls повторён успешно в static и dynamic builds.
Остальные 68 workloads
прошли, включая namespace/remove/load/debug, directory anchors и network proxy.
Это локальная проверка static configuration, не завершённая RV64 приёмка.
Новая архитектурная CI должна подтвердить target execution; до её завершения
исходный RV64 blocker не считается закрытым.

## 2026-11-15: удаление файлов через native namespace

Sys.remove выполняет один retained parent/name unlink либо empty-directory removal
вне namespace mutex и managed admission. Result publication guarded; эффект не
повторяется. Открытые FD/Module code живы после unlink; новый load image fails.
Четыре owners проверяют real rename/decoys, file bindings, shared result cell,
одного winner и zero live anchors. Port тестирует final/dangling symlinks,
empty/full dirs и восемь OS owners.69 mandatory workloads включают namespace-remove.
[RU](NAMESPACE-REMOVE.ru.md), [EN](NAMESPACE-REMOVE.md),
[пример](../examples/namespace-remove/README.ru.md).
Windows relative backend, full Chan/providers и union по-прежнему не заявляются.

## 2026-11-15: namespace в debugger по запросу

Команды namespace/namespace-read показывают root/cwd/import и все bindings
удержанного guest. Полные UTF-8 paths читаются bounded hex pages с mutation
revision; shared peer rebind делает старые pages недействительными. Нет новых
постоянных таблиц статистики, allocation/I/O/GC traversal; читается существующее
native state под mutex. Frozen source продолжает владеть своей группой.
68 mandatory workloads включают namespace-debug. Полные Chan/provider graphs,
FD/env/credentials и non-Linux adapters остаются работой.
[RU](NAMESPACE-DEBUG.ru.md), [EN](NAMESPACE-DEBUG.md),
[пример](../examples/namespace-debug/README.ru.md).

## 2026-11-15: удержание directory resources при rename/rebind

Root/cwd/import/directory bindings удерживают native handles; in-flight plans
сохраняют их до окончания I/O и освобождают при native unwind. FORKNS копирует
refs; NEWNS удерживает cwd как root. Четыре native owners проверены с настоящим
rename и подменой старого имени, чтением/create/import, независимыми cursors,
covering rebind и identity-filter unmount. Контроль реально читает подмену;
shutdown требует zero live anchors. Отдельно восемь OS owners проверяют relative
open, exclusive create и закрытие FD при позднем отказе fdopen без truncate.
Обязательный native-набор теперь67.
[RU](DIRECTORY-ANCHORS.ru.md), [EN](DIRECTORY-ANCHORS.md),
[пример](../examples/directory-anchors/README.ru.md).

File binding держит parent+basename; полный Chan/mount-parent walk, providers,
FD/env/credentials и non-Linux adapters остаются работой. Windows anchor backend
явно UNSUPPORTED; macOS/Android POSIX ещё требуют физической приёмки.

## 2026-11-15: namespace загрузчика и identity открытого образа

Файловый `load` разрешает namespace один раз до cache и native release.
Cache/open/parser получают frozen absolute host path; debug labels показывают
точный source. `chdir` обновляет import directory группы; FORKNS копирует его,
NEWNS сбрасывает в `/`. Bundled names теперь обычные file bindings, без
специального обхода namespace в `kopen`.

Исправлен hosted `kdirfstat`, прежде возвращавший нулевые identity/time.
Теперь metadata берётся с открытого файла. Проверены реальные cold/warm opens,
peer rebind/load во время native ожидания, atomic replacement с одинаковыми
length/time, старый живой Module и отказ cache при слишком широкой identity.
Omission zero metadata воспроизводит конкретный stale code со штатным retirement.
Обязательный native-набор теперь66. [RU](MODULE-NAMESPACE.ru.md),
[EN](MODULE-NAMESPACE.md), [пример](../examples/namespace-load/README.ru.md).

Публикация images — immutable files с atomic replacement; in-place edit при
неизменных секундных metadata не гарантирует обновления cache. Chan/provider
handles, полная revision/content identity, union/mount и остальные группы
окружения остаются следующими блоками. Исполняемые не-Linux adapters не заявлены.

## 2026-11-14: наследуемые native namespace

Добавлены `Sys.pctl(0/FORKNS/NEWNS, nil)`, `chdir`, `bind(MREPL)` и `unmount`.
`spawn` разделяет группу; FORKNS копирует, NEWNS берёт resolved cwd за новый
корень. Open/create/stat разрешают группу вызывающего; уже открытые FD стабильны.
Нет глобальной exclusive-секции. [Точные границы RU](VFS-NAMESPACE.ru.md),
[EN](VFS-NAMESPACE.md), [пример](../examples/namespace/README.ru.md).

Это host-path routing, ещё не Chan/provider VFS и не sandbox: symlinks,
rename/unlink identity, module-loader namespace/cache, union/mount providers,
FD/env/credential groups и dynamic extensions остаются работой следующих блоков.
Обязательный native-набор теперь65 с `namespace`. Linux x64 проверяется локально;
ARM64/RV64 требуют новой CI. Исполняемые Windows/macOS/Android adapters этим
блоком не перенесены; общее ядро namespace использует portable mutex API/libc.

Ниже сохранена история предыдущих checkpoints; старые количества workloads
и завершённые CI относятся к соответствующим прежним ревизиям.

## Текущий checkpoint: typed network proxies

[Trusted network owners](../remote/NETWORK-OWNERS.md) соединяют две реальные VM
через заранее установленный доверенный поток, включая loopback TCP. Проверяются
восемь клиентов, исходные variable/function owners, actual ref aliases, epoch,
cache/replay, разрыв после callback admission, malformed/truncated frames,
retirement callers, idle broker expiry и полный terminal root/IO cleanup.

Это не новый Dis64 loader/ABI, не owning dial/listen и не distributed GC.
Authentication и уникальный 128-bit connection epoch предоставляет transport/
launcher. Текущий manifest содержит 49 native workloads, 7 omission controls и
26 host-unit tests; под TSan добавляется контроль старой late-reply гонки.
Сетевой набор отдельно прошёл в C/JIT и обоих poll paths обычно и под TSan.
Расширенный полный TSan выявил raw scalar return race в Processctl.pid; она
исправлена guarded publication, с отдельными четырьмя writers/branch readers и
actual-race omission control. См. [точную границу builtin helper](BUILTIN-SCALARS.md).

Предшествующая матрица [6aedab6a](https://github.com/bhgv/flashVM/actions/runs/43439432556)
завершилась success во всех шести jobs: AMD64 plain/TSan, ARM64, RV64/QEMU и
два Dis64 Sys jobs. Она подтверждает прежние 48 workloads, не новые network cases.
QEMU не заменяет physical RV64 ordering/performance acceptance.

## Политика master

Активная разработка перенесена в `master`. Native-многопоточность без глобального
эксклюзива — обязательный основной и единственный поддерживаемый режим, включая
диагностические параметры. `--managed-workers` с любым значением, `--gc-inline`
и `--sequential-ui` отклоняются до старта VM. `--native-spawn` — совместимый явный
псевдоним штатного режима. Старые reference-ветви/fixtures сохраняются как история,
но штатная регистрация CTest и CI их не запускает. Локальный mutex poll остаётся
конкурентным вариантом polling, не остановкой мира.

Перенос не означает удаление всех исторических реализаций из upstream snapshots;
их запрещено реактивировать как режим исполнения или проверки.

## Приёмка первоначального переноса master

Перенос выполнен коммитом [`93a5154`](https://github.com/bhgv/flashVM/commit/93a5154a496c1f82de2f880ec999b4fa26097c15)
с сохранением истории проверенной ветки. До переноса `master` был её предком;
обновление выполнено fast-forward без force-push. Дальнейшие изменения публикуются
в `master`, workflow `.github/workflows/native-disvm.yml` отслеживает `master`.

Новый состав приёмки: 46 native-нагрузок, 4 native omission controls и 21 чистая
проверка host ABI/guards/генератора. Под TSan добавляется контроль старой гонки.
Локально весь состав прошёл **71/71 обычно и 72/72 под TSan**. Диагностические
`--managed-owner-probe` и `--native-queue-probe` дополнительно проверены на четырёх
постоянных гостях со строгой GC-политикой.

В [CI master](https://github.com/bhgv/flashVM/actions/runs/43389832715) AMD64 и ARM64
прошли по **46/46 native-нагрузок и 21/21 host-unit проверок**. AMD64 TSan и
RV64/QEMU ещё выполняются на момент этой записи; незавершённые jobs не считаются
успешными. Прежняя матрица 44/44 ниже относится к состоянию до переноса.

Добавлены проверки обязательного default-профиля и native-конструкторов на
40 постоянных владельцах. Старый shared/reference constructor oracle больше не
исполняется; от него используется только неизменный текст Limbo payload.
Исторические mixed/reference регистрации не подключаются. Числа 71/72 относятся
к новому составу, а не повторению старых 100 CTests. Чистые проверки Sys guard,
выбора платформы и генерации сохранены без запуска reference VM.

В этом переносе изменяется профиль запуска и приёмка, а не алгоритмы GC или
mailbox. Upstream snapshots не менялись. Изменённые исходники — не более 408 строк.

## Решение по этапу

**Рабочая native-многопоточная база для поддержанного консольного профиля
DisVM/Limbo реализована и прошла текущую приёмку. Можно переходить к прикладным
задачам на этой базе.** Дальнейший поиск гонок — сопровождение и проверка
конкретных нагрузок, а не бессрочное условие включения многопоточности.
Это решение ограничено описанным профилем; оно не объявляет весь исходный
проект, все расширения или каждое приложение полностью проверенными.

- Каждый `Prog` исполняется своим постоянным системным потоком с собственными
  живыми TLS-регистрами. Переноса гостя между исполнителями и восстановления
  рабочих регистров из `Prog.R` при пробуждении нет.
- Общий NativeWorld exclusive и остановленные проходы GC запрещены. Fast,
  Cycles и Duke работают конкурентно. Сохраняются локальные замки конкретных
  очередей, каналов, таблиц, дескрипторов и метаданных; это не режим без mutex.
- Запросы между потоками идут через mailbox. Исправлены подтверждённые ошибки
  публикации состояния/регистров, запоздалых ответов отмены и общей памяти
  уведомлений завершения детей.
- Обычный консольный запуск выбирает native-профиль по умолчанию. Для явного
  выбора при запуске приложения: `disvm --native-spawn --jit program.dis`.
  Выбор однопоточного reference-режима удалён.
- Неподдержанный legacy-путь не получает скрытый эксклюзивный fallback:
  попытка остановленного collector-пути явно завершается с кодом 70.

## Завершённая приёмка до переноса в master

Production-код: [`74fbab4`](https://github.com/bhgv/flashVM/commit/74fbab4f2aa066ba850554f9bcb5607d6832c8c0).
Последующий `c6159b2` уточняет комментарий и документацию; исполняемый C-текст
не меняется. [Полный завершённый CI](https://github.com/bhgv/flashVM/actions/runs/43384574676):

| Конфигурация | Результат |
| --- | --- |
| AMD64, системные потоки, C/JIT | 44/44 |
| AMD64, TSan, C/JIT | 44/44 |
| ARM64, настоящее оборудование runner, C/JIT | 44/44 |
| RV64, целевые ELF и JIT под QEMU | 44/44 |
| Дополнительный AMD64 constructor/channel oracle | Пройден обычно и под TSan |

Локально: 44/44 обычных нагрузок, 44/44 под TSan, 100/100 расширенных
native/boundary CTests и финальные 5/5 TSan-проверок с отрицательными контролями.
В workload входят несколько VM-запусков; это число сценариев, а не потоков.
TSan инструментирует C runtime, но не сгенерированные машинные инструкции JIT.

Проверяются строгая GC-политика, постоянные владельцы, результаты приложений,
передача ссылок/корней, каналы и I/O, отмена, retirement и join. Такой результат
не является доказательством отсутствия всех возможных гонок или измерением
ускорения. Список переносимых нагрузок: `native_arch_cases.py`, профиль `wide`.

## Отдельные следующие работы

| Работа | Граница текущего результата |
| --- | --- |
| RV64 на настоящем многопроцессорном железе | QEMU проверяет функциональность и ABI; hardware ordering/throughput ещё не проверены |
| Сторонние C callbacks/finalizers | Требуют собственного контракта владения и потоков; произвольные расширения не сертифицированы |
| GUI/editor и прикладные службы вне console-матрицы | Нужна приёмка соответствующего приложения/host |
| Оптимизация JIT и масштабируемости | ARM64/RV64 имеют настоящие native tail-входы к проверенному C executor; оптимизированное inline-покрытие всех opcode и ускорение не заявлены |
| FlashVM | Отдельный следующий проект; текущие изменения относятся к DisVM/Limbo |

Новые воспроизводимые дефекты поддержанного профиля исправляются как конкретные
ошибки. Сама возможность находить новые ошибки не удерживает остальные задачи
до завершения неограниченного аудита всей кодовой базы.

## English scope

The dedicated native-thread implementation is a usable, tested baseline for the
supported console DisVM/Limbo profile. All four architecture CI jobs completed
44/44; the local broader gate completed 100/100. This checkpoint closes baseline
implementation/acceptance work for that profile and permits application work.
It is not blanket certification of arbitrary extensions, GUI hosts, every VM
path, RV64 hardware ordering, JIT instruction instrumentation or speedup.
The remaining work above has separate, concrete acceptance targets.


2026-11-14: mandatory workload list now62 includes retained-debug and
jit-quarantine. Selected local ordinary/TSan regressions do not establish new
whole-matrix completion. Previous cf066b75 and47665a architecture CI runs
43605594611/43605884568 completed successfully. / Обязательный список теперь62;
новые retained-debug/jit-quarantine требуют своей полной CI-матрицы. Предыдущие
указанные CI завершились успешно.


2026-11-14: list63 adds jit-signal with actual RX traps, retained native CPU
context and unsafe-path refusals. Prior e123a54d CI43617004556 failed retained-debug
socket mode:0777 & ~0077 gives0700, not0600. Creation now uses0177 and a host-independent
mask/restoration probe; full AF_UNIX execution remains a CI check on this socket-denied
host. / RU: Список63; исправлена маска socket до0600, добавлена проверка umask
без AF_UNIX. Новую полную архитектурную матрицу нельзя считать пройденной заранее.


2026-11-14: prior CI43622919835 passed all six jobs including native ARM64 and RV64
QEMU. New list64 adds jit-fp (base FP/SIMD kernel copies, REAL RX faults and fenv
restoration); new matrix completion remains a separate observation. / Предыдущая
матрица прошла; новый FP-блок требует собственной архитектурной проверки.
