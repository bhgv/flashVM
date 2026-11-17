/* Copyright (c) 2026 bhgv
 * EN: Inject failure of one native outgoing admission while keeping the actual 9P/local queues.
 * RU: Ошибка принятия одного native TX при настоящих 9P и локальных очередях.
 */
#include "route/route.h"
static unsigned attempts; /* Atomic fixture counter, written only by the real TX relay. */


/* EN/RU: Actual native route state outside the injected failure; borrowed route, no mutation.
 * Настоящее состояние route вне инъекции ошибки; route заимствован, без изменений.
 */
extern int __real_styxbidirroutestate(StyxBidirRoute *route);


/* EN: Refuse outgoing admission once; count every retry to detect accidental replay/spin.
 * Borrow route/message/tag, publish a deterministic failed-transport observation, return 0.
 * RU: Отказать при принятии TX; считать все повторы для обнаружения replay/вращения.
 */
int __wrap_styxbidirrouteuisend(StyxBidirRoute *route, unsigned long tag, const DVM_UIMessage *message)
{
    (void)route;
    (void)tag;
    (void)message;
    __atomic_add_fetch(&attempts, 1, __ATOMIC_RELEASE);
    return 0;
}


/* EN/RU: Return WRITE_FAILED after refused admission; never alter the actual shared client.
 * После отказа TX вернуть WRITE_FAILED, не изменяя настоящий общий client.
 */
int __wrap_styxbidirroutestate(StyxBidirRoute *route)
{
    return __atomic_load_n(&attempts, __ATOMIC_ACQUIRE) ?
        STYX_CONTROL_WRITE_FAILED : __real_styxbidirroutestate(route);
}


/* EN/RU: Snapshot injected TX attempts after relay join; no queue/transport operation.
 * Копия числа попыток TX после join, без операций очереди/транспорта.
 */
unsigned terminalbridgefaultcalls(void)
{
    return __atomic_load_n(&attempts, __ATOMIC_ACQUIRE);
}
