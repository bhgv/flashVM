# Trusted-stream typed network owners

Copyright (c) 2026 bhgv

Две **настоящие DisVM** теперь обмениваются typed variable/function calls через
connected byte stream. В приёмке это два OS-процесса, настоящий loopback TCP,
восемь одновременных клиентов, C/JIT и оба register-poll paths. Код библиотек —
Limbo; функция исполняется исходным постоянным Prog, не C-моделью callback.

## Подключение и границы доверия

Порядок declarations: `sys.m`, `proxyhost.m`, `proxyref.m`, `proxyhandler.m`, `proxyowner.m`,
затем `proxynet.m` для клиента либо `proxyserver.m` для сервера.
Собрать matching NC64 producer библиотеки `proxyowner`, `proxyframe`,
`proxyref`, `proxystream`, `proxynet`, `proxyserver`, `proxyroute`: обычная native CMake-сборка уже включает
эти `.dis` в build. `init()` предшествует sharing/spawn.

`Proxyserver.serve(input, output, high, low, timeout, handler, cell, capture)`
создаёт отдельную session и исходного owner для пары exports. Клиентский
`Proxynet.start(input, output, high, low, timeout, ready)` запускается собственным
spawn. `ready` получает `(status, endpoint, variableImage, functionImage)`;
при отказе greeting status=Disconnected, endpoint=nil, images пусты.
`Proxynet.call` использует операции `Proxyowner.Read/Write/Invoke/Drop/Stop`.
Image содержит connection epoch и исходный owner handle. Проверяется target и
каждый использованный ref. Повторные refs сохраняют actual alias identity.

Направления FD исключительно принадлежат этому connection broker; caller
сохраняет FD живыми до completion/retirement. Проверенный socket путь — inherited
stdin/stdout, установленные trusted launcher. Публичного dial/listen здесь нет.
Новый Sys tuple gate Inferno не обходится и vendor bytes не меняются.

`high/low` — **128 бит уникального connection incarnation, предоставленные
launcher извне**. Их нельзя переиспользовать при reconnect/restart, даже если
новая VM повторила process-local session/slot/generation. В тестах константы
являются тестовыми данными; они не генератор production identities. Эпоха не
секрет и не authentication. Соединение должно быть доверенным/аутентифицированным
внешним transport. Открытый TCP без TLS/проверки peer этим API не становится
безопасным. Результаты не заявляют готовность untrusted public networking.

## Ограниченное состояние и исходы

Один connection broker последовательно передаёт запросы из bounded mailbox
capacity 16. Другие Prog и connections исполняются независимо. Исходный owner
держит actual Cell/capture/module roots; peer получает только handles и скаляры.
Direct nested `Proxyowner`/`Proxynet` RPC внутри active callback запрещён.
Явный routed callback обращается к переменным через `Proxyref`, чей независимый
actor выполняет только Read/Write/Add/Close с конечным deadline.
Retirement public caller не означает, что приватные IO/session children уже
joined: они удерживают FD/roots до своего completion/deadline. Направления нельзя
передавать новому broker до завершения этой частной session.

Сервер допускает только следующий положительный ID. Хранится **одна** пара
точных request bytes / completed response bytes. Точный last ID возвращает
cached outcome, включая callback error или Drop, без второго owner effect.
Тот же ID с другим payload даёт Conflict=6. Старый ID после продвижения окна
получает Stale=5; старый эффект больше не допускается. Переполнение ID закрывает
broker, не оборачивает счётчик. Это ограниченный reply cache без отдельного ACK;
после разрыва cache не переносится в новую session и automatic replay отсутствует.

Успешный reply означает completed owner effect. Callback exception возвращает
OwnerError=-1 и допускает частичный effect. Caller `NotSent=-2` относится к
локальному offer; `Unknown=-3` после offer не доказывает remote admission либо
отмену. Ошибка/повреждение/потеря wire reply после попытки передачи даёт Unknown
и навсегда закрывает этот broker. Следующие calls получают Disconnected=-4 до
нового wire IO. Broker публикует terminal state и завершается; reconnect создаёт
новый endpoint/images с новым epoch.

Caller timeout включает local offer/reply. Он не передаётся как синхронизированное
время другой машине: уже переданный запрос может выполнить effect после Unknown.
Blind retry запрещён. Wire exchange использует один local absolute monotonic
deadline для передачи и ответа; сервер делит один deadline между read, owner call
и response. Медленная передача одного байта не продлевает frame budget.
`timeout` также ограничивает idle lifetime client broker; истёкший endpoint не
возобновляется. Очистка/join зависит от OS scheduling и не является hard real-time.

Private I/O supervisor владеет своими native IO/alarm children до join. Если
public transfer caller уничтожен, buffered completion позволяет supervisor
дожить до deadline и завершить pending IO. Отдельная private server session
аналогично сохраняет cleanup при уничтожении public serve caller. При EOF,
framing failure, timeout или Stop session делает killwait original owner;
его native retirement hook завершает active pins и передаёт exports Fast.

`Proxystream.transfer` возвращает status и известный progress. При timeout progress
равен -1: syscall мог уже commit bytes до kill boundary. Buffer нельзя менять
до завершения supervisor. Такой stream закрывается логически, partial frame не
продолжается на другом соединении. Таймаут не превращается в доказанную отмену.

Native stdin/stdout socket readiness теперь зарегистрирована в cancellation
ledger. `poll` не держит queue mutex; `recv/send` используют `MSG_DONTWAIT`, send
также `MSG_NOSIGNAL`. FD flags не меняются. EOF/error/partial output восстанавливают
исходный heap owner и отпускают staging/root pins. Статистика native GC и banner
completion выведены в stderr, чтобы stdout мог нести только application bytes.

## Приватный формат версии 3

Все числа — явно восемь big-endian bytes; signed scalar — битовый 64-bit pattern.
Record имеет ровно 184 bytes, magic `DVP3`, version 3. Managed pointers, C padding
и bitmap/layout native frame в stream не попадают. Это private hosted NC64
протокол, **не DIS64 format, public kind/opcode или Word=4/ref=8 ABI**.

| Byte offset | Содержание |
| --- | --- |
| 0..7 | Magic 4 bytes, version, type (Hello=1/Call=2/Reply=3), reserved zero 2 bytes |
| 8 | Exact record size 184 |
| 16, 24 | Connection epoch high/low |
| 32 | ID; Hello=0, request/reply positive, максимум signed 63 bits |
| 40, 48, 56 | Operation (Read=1..Stop=5, Retain=6, ReturnVariable=7, ReturnFunction=8, Add=9, RoutedInvoke=10, ImportRoute=11), ref count 0..2, copied scalar value |
| 64, 88, 112 | Target/left/right: session, slot, generation по 8 bytes |
| 136, 144 | Status/copied scalar result; в request нули |
| 152 | Result kind: 0 absent, 1 variable, 2 function |
| 160, 168, 176 | Returned owner: session, slot, generation |

Hello передаёт variable в target и function в left. Slot ограничен 0..255,
owner session/generation в первом codec — неотрицательные signed 63-bit values;
реальные exports требуют ненулевых identities/generations. Reply должен точно
совпасть с request до byte 136, кроме type. Неизвестные status, epoch/version,
size/reserved, неверные ID/refs/slot и неполный кадр не становятся managed call.

## Ref переменной другой VM внутри исходного callback

`Proxyroute.start(sourceEndpoint, sourceImage, timeout, ready)` запускается на
своём permanent Prog. Он делает отдельный remote clone и передаёт `(status,
ref Proxyref.Ref)` через ready. Создаются bounded mailbox capacity 16 и private
native queue/token; ordinary Ref aliases разделяют token. Повторный start для
того же Image получает отдельное remote ownership. `proxyref.m` включает private
`proxygc.m`; его нужно объявить перед `proxyhandler.m`. Все libraries собираются
matching producer вместе с актуальными interfaces.

`Proxyowner.routed(functionEndpoint, functionHandle, leftRef, rightRef, refs,
value, timeout)` передаёт managed Ref arguments исходному function owner внутри
его VM. `Proxyhandler.routed` получает actual local capture и Proxyref arguments,
а не копии remote Cell. Native admission pin удерживает function capture; request
держит Ref tokens, каждый access держит свой source Ref до completion. Foreign
Cell pointer не пересекает heap/VM. Сам function callback исполняется исходным
Prog/TLS REG, его стек не переносится в actor либо network broker.

Callback использует `Proxyref.call(ref, operation, value, deadline)` для Read,
Write, Add либо Close. Это variable-only API; выполнить Invoke через actor нельзя.
Actor вне active function callback вызывает исходный `Proxynet` endpoint с
сохранённым private Image. Native connection/token identity проверяется перед
каждым dispatch, затем active state. Подстановка token другого actor в public Ref
даёт Stale даже при одинаковом source handle. Изменение полей Ref не меняет
private source Image actor. Обычные nested function RPC остаются запрещены.

Add (wire operation 9) вычисляет `target.value += input` на исходном variable
owner, затем возвращает actual value. Это один admitted primitive с обычным exact
request-ID replay cache. Повтор точного Add возвращает прежний outcome без второго
прибавления. Цепочка нескольких access, особенно к разным переменным, не становится
распределённой транзакцией; partial effects допустимы. Record остаётся DVP3/184
bytes; публичные Dis64 kinds/opcodes/layouts не изменены. Более старый private peer
может не поддерживать Add; protocol negotiation не заявляется.

Все access делят original absolute monotonic deadline callback. Queueing и
maintenance не продлевают бюджет; actor ограничивает source offer своим timeout.
Один access не передан до deadline — NotSent. После offer timeout означает Unknown:
исходный эффект может завершиться позже. Blind retry отсутствует. При потере source
связи actor публикует terminal state, отзывает wrapper tokens и завершает своё
ownership; новые calls получают Disconnected. Уже queued calls могут закончиться
Unknown по собственному deadline. Независимые owners/VM продолжают работу.

Actor хранит только свой source Image, channel/state и native Connection. После
ready он снимает local Ref/token copies. Поэтому последний Ref, включая Ref внутри
недостижимого цикла, может быть собран конкурентным GC, пока actor ждёт mailbox.
Native finalizer ставит только scalar obligation, actor снимает его и освобождает
ровно один remote clone. Явный Close делает это один раз и отзывает aliases;
другой actor того же source и отдельный source witness остаются живыми. Idle expiry
также освобождает clone. Finalizer не выполняет IO и не останавливает остальные
потоки. Actor/cleanup могут жить после caller retirement до конечного completion.

Приёмка: две настоящие VM, variable owner в A и два разных function owners в B,
два Ref actors к одной actual Cell, восемь callers и 32 атомарных прибавления.
Relay удерживает первый actual Add reply до входа двух разных original function
Prog в callbacks: правильная сумма при последовательном исполнении не проходит
такой gate. Capture PID и native lifetime evidence проверяют original owners.
Отдельный relay теряет ответ **после выполненного Add**: A сохраняет ровно один
эффект, оба callbacks получают error/Unknown, последующие accesses не replay,
а unrelated local owner B остаётся рабочим. Все Progs/roots/queues затем joined.
Private omissions native actor identity и actual GC Release обязаны обнаруживаться
явным semantic failure; crash/compile failure/timeout не являются доказательством.

`Proxyowner.routed` остаётся typed entry внутри function VM. Теперь внешний
caller третьей VM получает forwarding images через `Proxynet.importref`, а
`Proxynet.routed` передаёт их серверу функций. Его connection-scoped registry
проверяет exact handles и подставляет удерживаемые live Proxyrefs. Старый plain
Invoke/fetch с forwarding arguments отвергается до user callback: native stub
не выдаётся за actual foreign Cell. [Полный контракт трёх VM](FORWARDED-REFS.md).
Обычные wire ref-поля `Proxynet.call/fetch` по-прежнему требуют same-owner handles.

## Результаты-прокси из callback

`Proxynet.fetch(endpoint, function, left, right, refs, value, kind, timeout)`
вызывает `Proxyhandler.result` на исходном постоянном owner Prog. `kind` —
`Proxyhost.Variable` либо `Proxyhost.Function`. Callback получает actual capture
и до двух actual same-owner Cell refs, возвращает actual nonnil Cell. Сервер
публикует отдельный export root и возвращает handle; native pointer не передаётся.
Для function это capture того же typed handler module, не arbitrary closure ABI.
Обычный `invoke` по-прежнему возвращает скаляр.

Возвращённая переменная может быть новой Cell либо существующим ref: новый export
владеет отдельным root, а изменения alias видны через оба handle. Возвращённая
функция получает собственный capture root, исполняется тем же owner и остаётся
рабочей после Drop исходного function image. Прежние exports не заменяются.
Returned Image поддерживает обычные Read/Write/Invoke, clone, Drop и automatic GC
Release. Несколько клиентов могут одновременно получать и использовать results.

До wire callback broker резервирует native slot и managed token в состоянии
RESERVED, ещё без handle. Такой token нельзя activate/match/consume. После
проверенного успешного reply creating broker однократно привязывает exact handle
и делает token ACTIVE, без новой allocation. Ошибка, nil result, terminal reply
или отменённый caller не оставляют ложный active token. Unbound token собирается
без remote Drop; successful Image в abandoned buffered reply собирается с Drop.

Отказ локальной reservation даёт Capacity **до callback**. Отказ native export
quota на сервере возможен **после callback**: partial effects остаются видимыми,
Image не создаётся, Capacity не разрешает автоматический повтор. После explicit
Drop/collection освобождённого export следующий independent call может пройти.
`Proxyhandler.result` exception даёт OwnerError и также допускает partial effect;
nil Cell даёт Invalid. Session Stop/retirement очищает оставшиеся result exports.
Process-local `Proxyowner.fetch` возвращает scalar handle и требует explicit
Drop/Stop; автоматическое image ownership обеспечивает network adapter.

Reply cache хранит exact result bytes. Повтор того же ID возвращает **тот же**
handle, не вызывает callback ещё раз и не создаёт второй export/Retain. После
продвижения окна старый result request Stale. Disconnect/потеря/порча ответа
возвращает Unknown и не запускает replay. Remote session cleanup сохраняет roots
до завершения/retirement callback. Caller timeout не доказывает отмену эффекта.

В Hello и Call весь result extension нулевой. Failed/scalar Reply также обязан
иметь нулевые kind/handle. Successful result Reply требует kind requested operation,
нулевой scalar result, positive session/generation и bounded slot. Session должна
совпасть с invoked function. Malformed/foreign/wrong-kind result становится terminal
Unknown до adoption. DVP2 не принимается как DVP3; обе стороны собираются matching
producer с актуальными interfaces. Это private hosted формат, не Dis64 ABI.

## Владение Image и конкурентная сборка мусора

`Image` содержит managed lease token: обычные копии Image и `ref` aliases разделяют
одного логического владельца. `Proxynet.clone(endpoint, image, timeout)` создаёт
отдельного владельца через counted remote Retain. Variable и function используют
одинаковый механизм. Wire Retain проходит тот же bounded replay cache, что и Drop:
повтор точного request ID не увеличивает remote count второй раз.

Explicit Drop однократно потребляет lease до отправки. Все aliases этого lease
после этого получают Stale, а отдельный clone остаётся действующим. Session/slot/
generation сверяются с immutable native metadata lease: подмена public Image.owner
не даёт Drop/Invoke чужого export и не потребляет исходный token. Broker повторно
проверяет lease после извлечения запроса: queued call после Drop не допускается.
Request удерживает target/ref tokens до completion; активный call не теряет
владение из-за GC клиентского alias. Возвращаемый clone удерживается и buffered
reply: если caller исчез, недостижимый reply освобождает этот clone через GC.

Последний managed lease finalizes на конкурентном Fast. Недостижимый цикл с
вложенным Image обнаруживается Cycles и также отдаёт lease. Finalizer записывает
только scalar handle в bounded native queue под её локальным mutex: без сетевого
IO, managed allocation, callback и глобальной остановки. Broker между requests
передаёт Drop, используя следующий ID и обычный конечный wire deadline. Один
maintenance tick — до 100 ms; GC traffic не продлевает исходный idle deadline.
`collected(endpoint, timeout)` наблюдает число подтверждённых GC Releases, без
forced collection. Scheduling/collection не имеют hard real-time гарантии.

Очередь ограничена 256 slots, включая reserved/staged/live/pending tokens. Для clone slot
и managed token резервируются **до** remote Retain; capacity не создаёт удалённый
эффект. Token активируется только после успешного reply. Native storage удерживают
connection payload и каждый token независимо. Exact runtime-private descriptors
не дают обычному Limbo ADT подменить native payload. Публикация нового native
объекта использует зарегистрированный внешний root и guarded mapped result;
отказ root/publication сохраняет прежний result и освобождает native owners.

Terminal error/Unknown/Stop/idle закрывает metadata и отзывает все tokens.
Pending finalizer records удаляются; remote session retirement освобождает exports.
Неопределённый Drop/Retain не повторяется в новой session. Это per-connection
counted ownership с локальной RC/cycle collection, а не distributed tracing или
сборка циклов, проходящих через несколько узлов.

## Приёмка и оставшаяся работа

`runtime/test_network_proxy.py` проверяет два реальных TCP VM, alias updates,
исключение после partial effect, explicit Drop/Stop, replay после потерянного
response, conflict, stale window и новый epoch при повторившихся native handles.
Проверяются fragmented requests, truncated/malformed header/payload, повреждение
correlation/epoch/status ответа, отсутствие ответа, bad greeting без capability,
trickle deadline, обрыв после **actual callback entry**, native output backpressure,
неизменность FD flags, retirement public IO/serve callers и idle images.
Shared native scalar results дополнительно проверяются четырьмя writers и
branch readers: [builtin publication contract](../runtime/BUILTIN-SCALARS.md).
Network test обнаружил raw Processctl.pid return store; исправленная публикация
не повторяет native side effect после BUSY. TSan control возвращает raw store и
обязан показать actual race.
Каждая positive VM проверяет matched native resident joins, отсутствие миграции,
concurrent GC policy, пустые RC queues, exports=0/active=0 и native staging=0.
Проверены также native token quota, aliases, staged activation, запрет mutation
с чужого resident thread, повторное использование slot, сборка self-cycle, GC
original variable/function при живых отдельных clones, вызов retained function
после GC original с двумя live ref aliases, отдельный GC function clone без
отзыва variable ref, GC последнего variable clone и replay Retain.
Joined `PROXYGC` counters требуют queues/tokens/pending=0 и баланс drained/discarded.
Дополнительно проверены callback variable/function results, восемь concurrent
result clients, live returned ref alias, независимые roots после original Drop,
GC возвращённых images, exact result replay, обрыв после actual result callback
entry, malformed/foreign/wrong-kind/truncated reply, orphan result reply GC,
remote quota после partial effect и восстановление после освобождения export.
Private compiled omissions exact owner match / Retain / finalizer Release / epoch
/ duplicate cache / result validation / result binding обязаны дать конкретный
semantic failure; crash/compile failure/timeout не засчитываются. Отдельный
private refusal ограничивает только RESERVED allocation и подтверждает отсутствие
remote callback effect, сохранение output и чистое завершение session.

Доступны int Cell, copied scalar result, typed variable/function result images
и до двух same-owner variable refs в plain wire calls; registered routed callback
принимает до двух forwarding images от третьей VM и работает с live refs их owners. Нет arbitrary closure/value types,
String snapshots, arbitrary route discovery и transparent compiler lowering.
Обычные Image copies разделяют lease; отдельный owner создаёт explicit clone.
Это ещё не cross-node distributed tracing GC. TLS/peer authentication, automatic unique epoch
provisioning, owning dial/listen/FD adoption и полноценная cross-node lease/ACK
политика остаются отдельными границами. ARM64/RV64 выполняют ту же CI-приёмку;
QEMU не доказывает physical memory ordering или performance.


Client broker теперь сохраняет original absolute deadline в local Request,
включая время ожидания в mailbox. Просроченные queued operations отказывают до
lease consumption/result reservation/wire ID. Подробности, реальные late-effect
контроли и повторное использование forwarding actors: [QUEUED-DEADLINES.md](QUEUED-DEADLINES.md).
