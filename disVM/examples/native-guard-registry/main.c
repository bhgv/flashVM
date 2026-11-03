/* Canonical sidecars protect caller-owned native data, not arbitrary Dis heaps. */
#include "disvm/guard_registry.h"
#include "disvm/native_context.h"
#include <stdio.h>
#include <stdlib.h>


/* Stop on protocol misuse before continuing with an unowned guard or lease. */
static void require(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "guard example: %s\n", message);
        exit(1);
    }
}


/* Keep native payload on this stack until both independently acquired leases
 * retire. TLS is a bare native context, with no managed root/heap permissions.
 * Return0 after exact guard, lease, domain and context teardown; errors stop1. */
int main(void)
{
    int payload = 0;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    DVM_GuardRegistry *registry;
    DVM_GuardLease *first;
    DVM_GuardLease *second;
    DVM_ObjectGuard *guard;
    require(context && dvm_native_context_bind(context), "bind native TLS");
    registry = dvm_guard_registry_create();
    require(registry != NULL, "create registry");
    first = dvm_guard_registry_acquire(registry, &payload);
    second = dvm_guard_registry_acquire(registry, &payload);
    require(first && second, "independently acquire one identity");
    guard = dvm_guard_registry_guard(first);
    require(guard == dvm_guard_registry_guard(second), "canonical guard identity");
    require(dvm_object_guard_try_enter(guard) == DVM_OBJECT_ACQUIRED, "claim guard");
    /* Only payload work belongs in this interval: no VM calls, owner changes,
     * blocking waits or publication. Fetch guard pointers before acquiring. */
    payload += 7;
    require(dvm_object_guard_leave(guard), "release guard before ownership changes");
    require(dvm_guard_registry_release(&first), "retire first lease");
    require(dvm_guard_registry_guard(second) == guard, "second lease keeps guard live");
    require(dvm_guard_registry_release(&second), "retire final lease");
    /* guard is now invalid: never dereference or compare it for new ownership.
     * payload is still independently owned by this native stack frame. */
    require(dvm_guard_registry_destroy(&registry) && !registry, "retire registry");
    require(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "retire TLS");
    printf("PASS native canonical guard example: payload=%d\n", payload);
    return 0;
}
