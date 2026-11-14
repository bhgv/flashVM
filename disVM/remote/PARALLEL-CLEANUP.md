# Independent source cleanup / Закрытие независимых источников

`Proxyregistry.close` previously closed one view at a time, giving every view a
fresh full timeout. A slow source could hold unrelated sources open and multiply
the cleanup delay across up to sixteen outstanding views.

After the original function owner has joined, the registry now:

1. Marks the context closed and computes one absolute cleanup deadline.
2. Starts one dedicated cleanup child for every live view and immediately clears
   the registry's source roots. Each child owns its Ref through one Close under
   that shared deadline; exceptions do not suppress its completion publication.
3. Joins all cleanup children, then every outstanding original source actor.
   Completion channels carry only PIDs, never Ref/Image roots.

Cleanup closes independent sources concurrently. A single source connection still
uses its original single-flight broker and exact request IDs. The change adds no
replay, owner migration, global stop/exclusive or GC finalizer IO. Already admitted
work remains uncertain on loss. A shared request budget does not turn OS/native
thread retirement or GC progress into a hard real-time guarantee.

Раньше каждый view получал новый полный timeout после завершения предыдущего
Close. Теперь registry отзывается сразу, cleanup children получают один общий
absolute deadline и закрывают independent refs одновременно. Registry снимает
source roots до ожидания; children удерживают их до завершения access и передают
лишь PID. Затем выполняется join cleanup children и original source actors.
Потеря ответа не приводит к retry, migration owner либо глобальной остановке.

## Actual ordering proof / Проверка настоящего порядка

Three actual VMs participate: two original sources A1/A2 and a coordinator B.
B imports independent views into its actual function owner's native namespace,
then stops/joins that function owner before calling production registry.close.
Two fault relays hold each source's first completed clone Drop reply. Both Drop
requests must arrive before either reply is released. A fallback bound lets a
broken sequential implementation finish so it can fail on ordering, not timeout.

All original greeting owners are then dropped, both source brokers Stop/join,
the registry's actor slots and view roots are empty, and a second close is a no-op.
Both original source Cells remain 10; B's local original Cell remains 42. Drivers
require all permanent native-thread lifetimes, strict concurrent-GC evidence,
zero active/export/native RC work and drained proxy GC queues/tokens.

Обе source VM должны получить clone Drop до освобождения первого completed reply.
Это проверка actual happens-before, а не оценка скорости по wall-clock. При
private omission только cleanup spawn заменяется direct call: fallback освобождает
первый reply, все три VM завершаются чисто, но observer доказывает отсутствие
второго Drop до release первого. Compiler errors/crashes/timeouts не засчитываются.
C/JIT × fast/locked GC polling проверяются отдельно от этой намеренной omission.

A second positive relay variant loses source A1's completed clone Drop reply and
closes that link after observing both independent Drops. A2 must still finish
its ordinary greeting Drop/Drop/Stop sequence. B's closed A1 endpoint must refuse
all subsequent operations as Disconnected and its broker must join; A1's original
server observes EOF and cleans its remaining exports. All three VMs are checked
for native/root/lease retirement, with original values A1=A2=10 and B=42.

Ещё один positive variant теряет completed clone Drop reply A1 и закрывает связь.
A2 независимо завершает обычный Drop/Drop/Stop. Closed A1 endpoint отказывает
последующим calls как Disconnected; его broker и original server завершаются.
Это actual post-effect disconnect, а не имитация успешного ответа при потере связи.

## Verified series / Проверенная серия

2026-11-14: complete ordinary CTest passes 90/90. After adding the source-loss
variant, both affected ordinary retirement groups pass again 2/2. Final selected
TSan passes 11/11: network, forwarding, churn, routed results, deadlines, guest
proxies, retirement and their selected controls. The standalone TSan driver also
passes all widened terminal-publication, actual Ref Close/Read, parallel cleanup,
source-loss and existing orphan cases, plus every selected semantic omission.

Полный ordinary CTest: 90/90; после добавления source-loss: повтор 2/2.
Затронутые TSan группы: 11/11. Новый полный 91-test TSan результат не заявлен.
Architecture workload count остаётся 54; private retirement driver расширен.
Новая GitHub CI относится к новому опубликованному tree и проверяется отдельно.

## Known retired source at import / Завершившийся source при import

Registry import now checks the received Ref for nil, terminal state and native
lease activity before asking the original function owner to expose a new stub.
A source already known to have retired returns Unknown with an absent handle.
Retirement after this check remains an ordinary asynchronous revocation; the
check is not a lifetime pin or a guarantee that the remote source cannot fail.

The registry actor-failure test covers exceptions before Ready and inside the
post-Ready run loop, in C/JIT with both GC polling paths. A private observation
gate lets the post-Ready source settle before import continues. Both imports
must return Unknown without views; original source values remain 10, local value
42, actor slots and native/root ownership drain. This test now runs inside the
native retirement architecture workload, including CI, rather than only manually.

Import отказывает source, retirement которого уже известен: nil, terminal state
либо inactive lease проверяются до expose. Более поздний отказ source обрабатывается
обычным отзывом; новый guard не объявляется lifetime pin. Проверка early/post-Ready
exceptions включена в native retirement workload с actual тремя VM и clean joins.
