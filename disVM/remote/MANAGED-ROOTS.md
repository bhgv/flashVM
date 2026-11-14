# Managed export roots in native DisVM

Copyright (c) 2026 bhgv

`runtime/remote_managed.h` связывает bounded proxy sessions с настоящими Heap
объектами работающего native DisVM. Реализация подключена к runtime, имеет
производственный C API и отдельную проверку `--remote-owner-probe`.

## Владение

| Граница | Обязательство |
| --- | --- |
| Publish input | Caller уже удерживает валидный managed source и его root; произвольный адрес не становится валидным Heap |
| Publish success | Отдельные strong ref и ordinary registry root принадлежат export; caller сохраняет исходное владение |
| Publish failure | Добавленные ref/root откатываются; caller и output handle не меняются |
| Native peer | Видит handles и native request metadata; не получает Heap/payload pointers |
| Admission | Target и каждое вхождение variable ref получают pins, aliases сохраняются |
| Resolve | Только exact ACTIVE owner получает actual values конкретного active request; после finish — STALE без изменения output |
| Close/expiry | Новые admissions и peer refs отзываются; pins принятого запроса живут до finish |
| Collect | Только точный creating ACTIVE owner; до budget eligible roots передаются Fast |
| Queue refusal | Непереданный export остаётся rooted/owned в adapter и учитывается в общей quota, даже если session slot уже освобождён |
| Finalization | Конкурентный Fast получает существующий root вместе со strong owner; adapter не вызывает foreign destructor |
| Destroy | Только owner после join native callers, завершения запросов и передачи всех roots |

`dvm_runtime_rc_transfer_root` резервирует mandatory FIFO, затем передаёт ей
существующий registry root без extra root allocation, decrement или root gap.
До commit отказ сохраняет владение; после commit publisher не читает Heap.
Native session mutex не удерживается при GC/root/retirement operations.
Если collect передал часть roots до отказа, `released` сообщает эту часть;
CAPACITY сохраняет только оставшиеся roots. Повторяет collect именно owner.

Adapter ограничивает также detached roots, ожидающие retirement. Их нельзя
накапливать без границ, многократно переиспользуя уже пустые session slots.
Wrong-context и non-ACTIVE вызовы heap API отвергаются до dereference source.
Admission/finish/retain/release/tick/close работают с native metadata из других
потоков при независимой гарантии lifetime adapter до join.

`dvm_remote_managed_resolve` копирует request metadata и возвращает owner-local
borrowed values. Это не второй admission или invocation, не новый strong owner
и не разрешение читать shared payload без guards. Executor не завершает request
и не собирает exports до конца использования. Native peers не получают эту view;
wrong-context отказывает до доступа даже к output. Actual ref aliases проверены
на pointer identity после expiry; после finish новое resolve запрещено.

## Проверки

`test_remote_managed.py` выполняет настоящий NC64 DisVM с native OS-потоками и
конкурентными Fast/Cycles/Duke, C/JIT и обоими poll paths. В probe используются
два настоящих String exports, восемь native request peers, два alias refs на call,
expiry при активных pins, неизменные bytes после GC и возврат String Type count
к исходному значению после retirement. Обычный guest затем исполняется на своём
постоянном потоке; live TLS REG и native retirement проверяются по runtime evidence.

Private relink отказывает первой передаче в Fast, проверяет сохранение корней и
quota и последующее завершение. Три отдельные omission-сборки пропускают strong
retain, root registration или rollback. Probe отвергает их до unsafe access.
Production source/objects эти сборки не меняют; reference VM не используется.

В Sys capture исправлены два обычных refcount reads на общий atomic refcount API.
Host-unit проверяет 200000 captures с aliases параллельно 200000 retain/drop pairs.
Независимый private replay старых raw reads действительно даёт TSan data race.

## Оставшаяся граница

Function-kind String в probe проверяет только GC lifetime opaque owner token;
это не вызов функции или проверка closure captures. Native peers в этом probe не
исполняют Dis bytecode. Отдельный [guest path](GUEST-OWNER.md) теперь выполняет
настоящие typed Read/Write/Invoke через owner channel loop. Общий typed ReadAs,
cross-node ref routing, managed result cache/ack и network authentication остаются
отдельной работой. [Trusted network path](NETWORK-OWNERS.md) проверяет два реальных
DisVM peers, refs, terminal owner cleanup и managed image leases: explicit clone,
GC Release, collection вложенного self-cycle и независимость retained owner. Distributed GC или полный network proxy не
объявляются готовыми. Shared DIS64 kinds/opcodes не изменены.

Returned typed Cell/capture также получает отдельный managed export root на
исходном owner. Network client заранее резервирует token, затем binding проверяет
exact result handle без allocation. Result replay не создаёт второй root; GC
abandoned buffered result reply отправляет ровно один Release. Это проверено
real two-VM результатами и независимым wire peer, включая quota и disconnect.

Cross-owner Proxyref держит managed wrapper token, а независимый actor — private
source Image/clone. После ready actor не держит token; последний Ref GC может
поставить scalar Release obligation даже при actor, ожидающем channel. Requests
удерживают Ref до completion, function native pin удерживает только local capture.
Exact native queue identity запрещает подстановку token другого actor.

Forwarding image в C удерживает native variable stub B; registry B удерживает
отдельный Proxyref actor/clone A. Last image Drop/GC уменьшает native B ownership;
после последнего owner registry снимает Ref, его finalizer будит actor, который
снимает ровно один source clone A. Import reply переживает caller retirement как
buffered managed Image, а затем участвует в том же transitive release.


Routed result Request дополнительно держит managed `RoutedResult.source` до
completion. Callback возвращает borrowed Ref, затем registry проверяет exact live
wrapper и делает native Retain уже существующего B view handle. Managed root
защищает временный Ref; только Retain создаёт независимое remote peer ownership.
C result reservation/bind и GC остаются прежними: [ROUTED-RESULTS.md](ROUTED-RESULTS.md).
