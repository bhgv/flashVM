/* Copyright (c) 2026 bhgv
 * EN: Native capability claim, explicit inheritance, owner admission and retirement contracts.
 * RU: Native-capability: claim, явное наследование, допуск владельца и retirement.
 */
#include "disvm/unibus_binding.h"
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Stack-owned fixture outlives every retained binding; no VM pointers.
 * Fixture на стеке живёт дольше всех refs, указателей VM нет.
 */
typedef struct State
{
    unsigned calls; /* Accepted native callbacks. */
    unsigned retired; /* Once-only root cancellation callback. */
    unsigned destroyed; /* Once-only final native disposal callback. */
} State;


/* EN/RU: Fail visibly in every compiler profile. / Явная ошибка в любом compiler profile.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "Unibus binding: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Borrow native state/frame and copy one result; this fixture has no VM ABI dependency.
 * Заимствовать native state/frame, записать результат; fixture не зависит от VM ABI.
 */
static int invoke(void *opaque, int pid, unsigned method, void *frame)
{
    State *state = opaque; /* Caller-owned fixture. */
    if (method == UNIBUS_INK)
    {
        return 0;
    }
    state->calls++;
    *(int *)frame = pid + (int)method;
    return 1;
}


/* EN/RU: Count root cancellation without freeing state. / Считать отмену root без free state.
 */
static void retire(void *opaque)
{
    ((State *)opaque)->retired++;
}


/* EN/RU: Observe final disposal; caller still owns stack storage. / Наблюдать последний dispose.
 */
static void dispose(void *opaque)
{
    ((State *)opaque)->destroyed++;
}


/* EN/RU: Exercise opaque owner identities independently from potentially reused numeric PIDs.
 * Проверить opaque owner identities независимо от потенциального повторения PID.
 */
int main(void)
{
    State state = {0}; /* Lives through both capability lifetimes. */
    UnibusOps ops = {invoke, retire, dispose}; /* Native callbacks, copied on success. */
    int frame = 999; /* Output sentinel for refusals. */
    check(!unibusbinding(NULL, &state, 0) && !unibusbinding(&ops, &state, 8) && !state.destroyed,
        "invalid construction preserves caller state");
    UnibusBinding *binding = unibusbinding(&ops, &state, UNIBUS_INHERIT | UNIBUS_OWNER_PORTS);
    check(binding && unibusinvoke(binding, 10, 7, UNIBUS_ROWS, &frame) == -1 && frame == 999,
        "unclaimed binding cannot execute");
    check(unibusclaim(binding, 10) && !unibusclaim(binding, 11), "one native root identity");
    UnibusBinding *child = unibusinherit(binding); /* Independently retained delegated capability. */
    check(child == binding, "explicit inheritance retains capability");
    check(unibusinvoke(child, 11, 7, UNIBUS_ATTACH, &frame) == -1 && !state.calls && frame == 999,
        "same numeric PID does not confer another native owner's rights");
    check(unibusinvoke(child, 11, 8, UNIBUS_ROWS, &frame) == 1 && frame == 8 + UNIBUS_ROWS,
        "delegated operation reaches its own native handler");
    check(unibusinvoke(child, 11, 8, UNIBUS_INK, &frame) == -3, "unavailable operation is explicit");
    unibusretire(child, 11);
    check(!state.retired, "child retirement cannot revoke root");
    unibusretire(binding, 10);
    unibusretire(binding, 10);
    check(state.retired == 1 && !unibusinherit(binding), "sticky once-only root revocation");
    unsigned calls = state.calls; /* No callback may be admitted after completed revocation. */
    check(unibusinvoke(child, 11, 8, UNIBUS_ROWS, &frame) == -2 && state.calls == calls,
        "retired delegated reference cannot invoke");
    unibusrelease(binding);
    check(!state.destroyed, "child retains native state after root release");
    unibusrelease(child);
    check(state.destroyed == 1, "final native state disposed exactly once");
    binding = unibusbinding(&ops, &state, 0);
    check(binding && unibusclaim(binding, 12) && !unibusinherit(binding), "inheritance disabled by default");
    unibusretire(binding, 12);
    unibusrelease(binding);
    check(state.retired == 2 && state.destroyed == 2, "independent capability lifetimes");
    binding = unibusbinding(&ops, &state, UNIBUS_INHERIT);
    check(binding && unibusclaim(binding, 20), "shared service creation");
    child = unibusinherit(binding);
    unibusownerretired(binding, 20);
    unibusrelease(binding);
    check(state.retired == 2 && state.destroyed == 2 &&
        unibusinvoke(child, 21, 9, UNIBUS_ATTACH, &frame) == 1,
        "inherited shared service survives initial thread exit with delegated rights");
    unibusrelease(child);
    check(state.retired == 3 && state.destroyed == 3, "last inherited ref closes shared service");
    binding = unibusbinding(&ops, &state, UNIBUS_INHERIT | UNIBUS_OWNER_PORTS);
    check(binding && unibusclaim(binding, 30), "owner-only optional policy");
    child = unibusinherit(binding);
    unibusownerretired(binding, 30);
    unibusretire(child, 30);
    check(unibusinvoke(child, 30, 9, UNIBUS_ATTACH, &frame) == -1 &&
        unibusinvoke(child, 31, 9, UNIBUS_ROWS, &frame) == 1,
        "reused identity cannot regain retired owner-only rights or revoke the service");
    unibusrelease(binding);
    unibusrelease(child);
    binding = unibusbinding(&ops, &state, UNIBUS_INHERIT | UNIBUS_CLOSE_WITH_OWNER);
    check(binding && unibusclaim(binding, 40), "explicit owner-bound lifetime policy");
    child = unibusinherit(binding);
    unibusownerretired(binding, 40);
    check(unibusinvoke(child, 41, 9, UNIBUS_ROWS, &frame) == -2,
        "owner-bound lifetime revokes children only when explicitly requested");
    unibusrelease(binding);
    unibusrelease(child);
    check(state.retired == 5 && state.destroyed == 5, "all native policy lifetimes drained");
    puts("PASS native Unibus binding ownership and isolation");
    return 0;
}
