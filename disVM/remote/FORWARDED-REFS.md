# Forwarding refs across three DisVMs

Copyright (c) 2026 bhgv

EN: A connection-scoped registry gives a caller opaque variable images that retain
live storage in another VM. Functions stay on their original native owner threads.
RU: Connection-scoped registry даёт caller непрозрачные variable images с живым
storage другой VM. Функции остаются на своих исходных native owner threads.

## Роли и API

| VM | Что она удерживает и исполняет |
| --- | --- |
| A | Actual Cell, native export root и исходный variable owner |
| B | Source connection A, registered templates, независимые ref actors и original function owner |
| C | GC-owned images в namespace B; callback requests с этими images |

B создаёт array `ref Proxyregistry.Source(endpointA, imageA)` и передаёт его
`Proxyserver.registered`. Session копирует records до начала обслуживания;
произвольные адреса/URL и handles других owners из wire не открывают connections.
Порядок interfaces: Sys, Proxyhost, Proxyref, Proxyhandler, Proxyowner, Proxynet,
Proxyregistry, Proxyserver. `init()` предшествует sharing/spawn.

C получает variable/function greeting B обычным `Proxynet.start` и вызывает:

```limbo
(status, forwarded) := net->importref(endpointB, functionB, 0, timeout);
(status, value) = net->routed(endpointB, functionB,
    forwarded, forwarded, 2, delta, timeout);
```

Slot 0 обозначает только первый заранее настроенный Source. Неверный slot даёт
Invalid без usable image. Import создаёт отдельный actor/clone A и native variable
stub B, удерживаемый native export root. Stub публикуется на original B owner под
проверенным function pin, без user callback. C видит только exact native B handle.
Новый image имеет отдельный GC token, зарезервированный **до wire Import**.

Read/Write/Add такого image выполняются через actor в actual A storage. Два
самостоятельных imported images одного Source видят одну переменную. Callback B
получает typed Proxyrefs к этим original owners. Старый plain Invoke/fetch с такими
ref arguments возвращает Invalid: stub Cell с внутренним индексом не превращается
в foreign Cell и не передаётся user callback. Любой другой target/used ref должен
пройти exact session/slot/generation lookup; stale view не выбирает replacement.

## Владение, GC и завершение

Обычные C Image copies разделяют lease, `clone` делает отдельный counted Retain B.
Registry зеркалит подтверждённые native Retain/Drop. Original Drop не отзывает
retained image. Последний Drop/GC image снимает native B ownership и registry Ref;
native Ref finalizer ставит scalar obligation, actor освобождает свой clone A.
Ни один finalizer не выполняет IO/managed callback и не останавливает мир.
Source registration сама владеет своим Image A до завершения service; это отдельное
владение, которое не исчезает вместе с одним imported view.

Buffered Import reply удерживает новый Image даже после исчезновения caller C.
После получения ответа broker C может создать image, отправить его в abandoned
buffer и снять свои local copies; GC затем выполняет ту же цепочку C→B→A.
Неполученный/повреждённый reply не разрешает blind retry. Connection cleanup
закрывает B original owner, отзывает оставшиеся refs и join всех source actors.
Native B stubs освобождаются его retirement hook. Исчезновение public serve caller
не уничтожает private session, владеющую этим cleanup.

Контекст ограничен 16 configured sources, 16 view slots, 256 issued actors и
256 peer owners на view. Native owner имеет собственный object quota, поэтому с
двумя initial exports обычно одновременно проходят 14 новых stubs. Отказ native
publication после source clone не возвращает usable view: actor остаётся учтённым
для GC/join. Local C result quota отказывает до wire request. Это counted ownership
с локальной RC/cycle collection, не tracing циклов между несколькими VM.

## Дедлайны и плохая связь

Original B frame deadline передаётся registry, source startup и native owner
routed/expose offers без обновления после allocation/queueing. Proxyroute.startuntil
разделяет absolute startup budget и последующий actor idle/exchange timeout.
Caller C и source A используют свои local monotonic budgets; синхронные часы
между VM не требуются. Heap/OS scheduling не даёт hard real-time гарантии.

После offer Unknown не доказывает отмену эффекта. Если A выполнила Add, а ответ
потерян на A↔B, callback B получает error/Unknown; unrelated B storage ещё usable,
следующий access потерянного Source закрывает C connection. Если потерян B↔C,
B может завершить callback и его effects; C всё равно получает Unknown. Ни один
из этих исходов не вызывает automatic replay. Несколько primitive accesses не
являются distributed transaction и допускают partial effects.

Private DVP3 по-прежнему имеет 184 bytes. Добавлены RoutedInvoke=10 и ImportRoute=11;
успешный Import использует существующий variable result kind=1/Returned handle.
Exact last-ID replay возвращает тот же handle/outcome без нового actor, export,
Retain или callback. Changed payload конфликтует; старый ID после window advance
Stale. Оба endpoints требуют matching libraries; feature negotiation не заявлен.
Public Dis64 kind/opcode/layout этим не меняется.

## Два независимых соединения в одной VM

B использует connected stream socket stdin для A и отдельный socket stdout для C,
каждый в обоих направлениях. Обратные направления (write stdin/read stdout)
допускаются только после проверки connected SOCK_STREAM. Regular files/FIFO
сохраняют прежние ограничения. Runtime не меняет descriptor flags, не принимает
произвольные native FD numbers и не владеет OS handles 0/1: host удерживает их до
завершения VM. Native waits cancellable, transfer выполняется с MSG_DONTWAIT /
MSG_NOSIGNAL; managed FD/array owners удерживаются через native staging.
Stderr остаётся diagnostics, stdout native B содержит только C protocol bytes.

## Проверки

Три actual VM выполняют imports, Read, восемь callers, 16 callbacks и 32 Add:
A меняется с 10 до 42, B local Cell остаётся 0. Проверяются aliases, shared Retain,
original Drop, GC C images и GC B Ref wrappers, native counters и joins всех Progs.
Отдельный relay держит completed Import reply до retirement caller C, затем
наблюдается transitive release и работа следующих views. Independent byte oracle
проверяет import/callback/Retain replay, payload conflict, wrong session/generation,
replacement identity и final Release. Loss gates разрывают каждый hop после actual
A effect. Private omissions slot authority, raw-stub exclusion, retained ownership,
result binding и native GC obligations обязаны дать конкретный semantic failure.
Отказ reservation и expired startup доказываются отсутствием wire Import/Retain;
crash/compiler failure/timeout не выдаются за такое доказательство.

TLS/peer authentication, dynamic route discovery/delegation, general closure/value
arguments и distributed cycle tracing остаются отдельными задачами. Этот entry
покрывает explicit server-authorized variable views и routed scalar callbacks.


Проверенный локальный результат этой серии: optimized CTest 82/82, полный TSan
CTest 83/83. Отдельный TSan запуск omission controls включает transitive B GC и
expired startup/no-Retain. Предыдущая опубликованная серия `b3bf5035` имеет шесть
успешных CI jobs (run 43487651642); новая публикация запускает собственную matrix.
ARM64/RV64 результаты предыдущей серии не заменяют проверку новой ревизии.
