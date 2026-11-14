# Live variable results from routed callbacks

Copyright (c) 2026 bhgv

EN: Original function owner B returns a live registered variable of A as a counted
image to caller C. Callback completion and result ownership are separate steps.
RU: Original function owner B возвращает live registered variable A как counted
image caller C. Callback completion и result ownership — отдельные шаги.

## API и смысл результата

```limbo
(status, image) := net->routedfetch(endpointB, functionB,
    firstView, secondView, 2, value, timeout);
```

`Proxyhandler.routedresult` исполняется на том же постоянном Prog, что и исходная
функция, и возвращает `ref Proxyref.Ref`. Callback получает capture, два typed refs,
их used count, scalar value и исходный absolute deadline. Variable-only operations
по этим refs идут через независимые actors к actual storage A. Callback не получает
foreign Cell, pointer в heap A или transport FD.

Результат должен быть точным live wrapper одной из registered views текущего
B connection context. Обычные ref copies сохраняют identity. Новый wrapper даже
с тем же mailbox/state/lease не становится зарегистрированной capability. Nil,
closed/inactive и незарегистрированный Ref дают Invalid=3 без usable Image.
Уже отозванная view не может быть заново делегирована через сохранённый handler ref.
Эта версия возвращает variable aliases зарегистрированных owners; она не публикует
произвольный новый Ref destination и не вводит general closure/function results.

## Два этапа владения

C сначала резервирует inactive native result token, затем отправляет request.
При отказе reservation нет wire callback и нет partial effect. Request удерживает
исходные Image leases, а local B Request — typed Proxyrefs и managed result cell.
Original function capture pin остаётся активным во время callback; finish pins
предшествует reply. Managed result cell сохраняет returned Ref при смене thread,
GC и retirement ожидающего caller, но сама не создаёт peer ownership.

Registry проверяет exact wrapper/active lease и выполняет native Retain B handle
на его owner до исходного deadline. Лишь подтверждённый Retain увеличивает mirrored
view owners и позволяет выдать result kind=1. Source actor A не клонируется ещё раз:
новый peer owner B удерживает ту же view и её исходный actor/clone A. Bound result
Image C получает отдельный logical lease; copies этого Image разделяют lease.

Original argument Drop и function image Drop не отзывают этот result. Последний
Drop/GC result снимает его native B peer owner. Когда больше нет peer owners view,
Ref wrapper GC передаёт scalar Release своему actor, который освобождает clone A.
Source registration template имеет отдельное владение до shutdown service.
Удержание Image не отменяет прежние connection/actor idle leases и их failure rules.

## Ошибки, deadline и повтор

Callback может изменить A и затем бросить exception: OwnerError=-1 возвращает
empty image, partial effects сохраняются. Failure native Retain/peer quota после
callback также не откатывает effects. Client reservation quota — до callback,
server-side result admission — после callback. Multi-access distributed transaction
не заявляется. Проверки matching function kind, exact sessions/generations и used
refs сохраняются до исполнения callback.

Original B deadline проходит callback offer, managed result allocation и final
native Retain без обновления. Known expired owner queue work не запускается.
Unknown после offer/обрыва не разрешает blind retry и не доказывает rollback.
При потере completed reply C не принимает result image, connection становится
terminal; B connection cleanup отзывает все session peer owners, включая result.

Если исчез лишь ожидающий caller C, broker продолжает already offered exchange,
принимает result в buffered orphan reply и GC освобождает ровно это новое владение.
Исходные argument images остаются usable, включая настоящий native Retain после
orphan result GC. Last-ID exact replay возвращает тот же handle/response и не делает
второй callback/Retain. Changed payload conflicts; после window advance старый ID stale.

Private DVP3 остаётся 184 bytes: RoutedReturnVariable=12 в wire, local enum=11.
Result использует прежние Returned handle/result kind=1 и scalar Result=0.
Owner-local Expose теперь enum=12, local client GCProgress=13; они не являются
wire operations. Proxyhandler/Proxyowner internal module interfaces расширены;
нужны matching producer/libraries. Public Dis64 kinds/opcodes/layout не меняются.

## Acceptance

Три actual VM проверяют returned native handle identity, distinct result lease,
Read/Write исходного A storage, original argument Drop и восемь concurrent callers.
Native clone после original Drop проверяет реальное peer ownership B: одной
managed registry shadow для этого недостаточно. Проверяются nil/unregistered
wrapper, wrong function kind, exception с одним actual Add и function image Drop.
Последний retained result помещается в unreachable self-cycle. Требуются девять
C result GC Drops, два B wrapper GC obligations, clean native roots/joins и B local=0;
A заканчивает со значением 42.

Independent byte oracle повторяет result request, проверяет exact response и
A=14 после единственного callback. Два Drop обязаны сделать native handle stale;
лишний replay Retain обнаруживается. Failure path меняет A ровно до 15.
Другой relay удерживает completed result до retirement caller, затем проверяются
orphan GC и живое исходное native ownership. Loss relay отбрасывает completed reply:
C видит Unknown/empty/disconnected, A остаётся 12, effects не повторяются.

Private omissions native Retain, exact wrapper lookup и client result binding
обязаны дать конкретные semantic failures с подтверждёнными A values и clean server
joins. Reservation refusal доказывается независимым peer: первый wire request Read,
а не RoutedReturnVariable. Crash, timeout и compiler error не считаются acceptance.


## Retirement caller и alarm threads

В Net.offer теперь private supervisor владеет alarm и ждёт original request/reply.
Проверка canwait остаётся на исходном caller до spawn, поэтому active function
callback не получает обход запрета nested RPC через новый Prog. Original deadline
вычисляется до allocation и не обновляется в supervisor или broker queue.

После completion supervisor завершает/join свой alarm, затем публикует buffered
completion public caller. Retirement public caller не оставляет sleeping alarm до полного deadline;
result в abandoned completion channel проходит тот же Image GC release. Already
offered work при этом не replay/cancel-transaction и всё ещё ограничено deadline.
Orphan acceptance оставляет request deadline 60 seconds, но требует выхода всей C
VM за 40 seconds, включая все native Progs. Раньше orphan alarm ждал 60 seconds
даже после успешного logical cleanup; новый supervisor устраняет это ожидание.


## Локальные Owner/Ref после retirement caller

Та же схема применяется к `Proxyowner.offer` и `Proxyref.call`: private supervisor
удерживает request/access, исходный source и alarm до reply либо original deadline.
Buffered completion не требует живого public caller. Supervisor завершает alarm
до публикации результата; обычный caller также join своего supervisor.

Owner проверяет `host.canwait` на исходном caller до spawn. Ref проверяет operation,
absolute deadline, closed state и active lease до spawn; отдельная source actor
по-прежнему выполняет доступ у настоящего владельца. Callback может использовать
разрешённый Ref access, но не получает право произвольного nested Owner RPC.
При отказе до offer эффект не происходит; retirement после offer не означает
отмену, откат или разрешение повторить запрос. Raw Owner handles сохраняют явное
session/Drop ownership: это исправление lifetime запросов, не новая GC-модель handles.

`test_proxy_retirement.py` проверяет C/JIT × fast/locked GC polling:

- Local function callback блокируется после фактического входа. Caller уничтожается,
  callback освобождается и меняет original Cell ровно с 10 на 11; Read/Stop и все
  native Progs должны завершиться за 40 seconds при 60-second request deadline.
- Две настоящие VM выполняют Ref Add. Relay видит успешный A=11, удерживает reply
  до уничтожения access caller, затем разрешает Read/Close/Drop/Stop. Проверяются
  independent clone ownership, rooted source и полное завершение VM за 40 seconds.
- Callback пытается вызвать Owner RPC. Production возвращает OwnerError на исходном
  callback. Private omission только pre-spawn canwait обязана вызвать конкретную
  semantic failure с clean owner roots; crash/timeout не считаются acceptance.

Все эти проверки используют обязательный native dedicated-thread режим без
exclusive/stop. Locked GC polling здесь означает проверку доступа к GC metadata,
а не глобальную остановку или альтернативный однопоточный executor.


## Проверенная серия

2026-11-14: полный обычный CTest прошёл 88/88, полный TSan CTest — 89/89.
После supervisor cleanup повторно прошли все восемь затронутых сетевых групп
обычной и TSan сборок, включая original deadlines, replay/GC/ownership omissions
и новые routed results. Orphan result сохраняет 60-second request deadline при
40-second whole-VM bound. Новая architecture matrix содержит 53 native workloads;
результаты новой GitHub CI отслеживаются отдельно от локальных и предыдущей серии.
Предыдущая `cacb7976` имеет все шесть successful jobs в run 43513106849.


2026-11-14, Owner/Ref retirement: полный обычный CTest прошёл 90/90.
Новые positive и policy-omission группы TSan прошли 2/2, включая обе actual
orphan операции во всех четырёх C/JIT/poll сочетаниях. Architecture matrix
расширена до 54 native workloads. Полный TSan rerun и новая CI проверяются
отдельно; pending execution не считается подтверждённым результатом.
Baseline `677b8069`: в run 43567770403 уже successful AMD64 plain/TSan, ARM64
plain и обе DIS64 Sys producer/owner группы; RV64/QEMU ещё выполняется.
