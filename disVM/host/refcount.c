/* Native atomic counts; unconverted VM/JIT paths still require exclusive ownership. */
#include "disvm/refcount.h"
#include "channel_sync.h"
#include <limits.h>

#if !defined(DVM_REFCOUNT_FORCE_LOCKED) && defined(__GCC_ATOMIC_LONG_LOCK_FREE) && __GCC_ATOMIC_LONG_LOCK_FREE == 2
#define DVM_REFCOUNT_ATOMIC_U 1 /* Use always-lock-free native unsigned-long operations. */
#endif
#if !defined(DVM_REFCOUNT_FORCE_LOCKED) && defined(__GCC_ATOMIC_INT_LOCK_FREE) && __GCC_ATOMIC_INT_LOCK_FREE == 2
#define DVM_REFCOUNT_ATOMIC_I 1 /* Use always-lock-free native signed-int operations. */
#endif


/* See public header. No pointer/header inference can manufacture object lifetime. */
unsigned long dvm_refcount_load_u(const unsigned long *counter)
{
    unsigned long result;
    if (!counter)
    {
        return 0;
    }
#ifdef DVM_REFCOUNT_ATOMIC_U
    result = __atomic_load_n(counter, __ATOMIC_ACQUIRE);
#else
    fvm_port_registry_lock();
    result = *counter;
    fvm_port_registry_unlock();
#endif
    return result;
}


/* CAS refuses zero/overflow on every retry. Acquire/release RMW is deliberately
 * stronger than minimal retain ordering; it is shared with the last-drop rule. */
DVM_RefcountResult dvm_refcount_retain_u(unsigned long *counter)
{
    unsigned long value;
    if (!counter)
    {
        return DVM_REFCOUNT_REFUSED;
    }
#ifdef DVM_REFCOUNT_ATOMIC_U
    value = __atomic_load_n(counter, __ATOMIC_ACQUIRE);
    for (;;)
    {
        if (!value || value == ULONG_MAX)
        {
            return DVM_REFCOUNT_REFUSED;
        }
        if (__atomic_compare_exchange_n(counter, &value, value + 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        {
            return DVM_REFCOUNT_LIVE;
        }
    }
#else
    fvm_port_registry_lock();
    value = *counter;
    if (value && value != ULONG_MAX)
    {
        *counter = value + 1;
    }
    fvm_port_registry_unlock();
    return value && value != ULONG_MAX ? DVM_REFCOUNT_LIVE : DVM_REFCOUNT_REFUSED;
#endif
}


/* The successful RMW reads the preceding modification chain. LAST therefore
 * observes writes released by previous owners before caller-side retirement. */
DVM_RefcountResult dvm_refcount_release_u(unsigned long *counter)
{
    unsigned long value;
    if (!counter)
    {
        return DVM_REFCOUNT_REFUSED;
    }
#ifdef DVM_REFCOUNT_ATOMIC_U
    value = __atomic_load_n(counter, __ATOMIC_ACQUIRE);
    for (;;)
    {
        if (!value)
        {
            return DVM_REFCOUNT_REFUSED;
        }
        if (__atomic_compare_exchange_n(counter, &value, value - 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        {
            break;
        }
    }
#else
    fvm_port_registry_lock();
    value = *counter;
    if (value)
    {
        *counter = value - 1;
    }
    fvm_port_registry_unlock();
    if (!value)
    {
        return DVM_REFCOUNT_REFUSED;
    }
#endif
    return value == 1 ? DVM_REFCOUNT_LAST : DVM_REFCOUNT_LIVE;
}


/* Signed Type reference words are distinct from native unsigned Heap words. */
int dvm_refcount_load_i(const int *counter)
{
    int result;
    if (!counter)
    {
        return 0;
    }
#ifdef DVM_REFCOUNT_ATOMIC_I
    result = __atomic_load_n(counter, __ATOMIC_ACQUIRE);
#else
    fvm_port_registry_lock();
    result = *counter;
    fvm_port_registry_unlock();
#endif
    return result;
}


/* Do not wrap signed counters or turn corrupt negative values into owners. */
DVM_RefcountResult dvm_refcount_retain_i(int *counter)
{
    int value;
    if (!counter)
    {
        return DVM_REFCOUNT_REFUSED;
    }
#ifdef DVM_REFCOUNT_ATOMIC_I
    value = __atomic_load_n(counter, __ATOMIC_ACQUIRE);
    for (;;)
    {
        if (value <= 0 || value == INT_MAX)
        {
            return DVM_REFCOUNT_REFUSED;
        }
        if (__atomic_compare_exchange_n(counter, &value, value + 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        {
            return DVM_REFCOUNT_LIVE;
        }
    }
#else
    fvm_port_registry_lock();
    value = *counter;
    if (value > 0 && value != INT_MAX)
    {
        *counter = value + 1;
    }
    fvm_port_registry_unlock();
    return value > 0 && value != INT_MAX ? DVM_REFCOUNT_LIVE : DVM_REFCOUNT_REFUSED;
#endif
}


/* A rejected signed drop preserves diagnostic negatives and zero exactly. */
DVM_RefcountResult dvm_refcount_release_i(int *counter)
{
    int value;
    if (!counter)
    {
        return DVM_REFCOUNT_REFUSED;
    }
#ifdef DVM_REFCOUNT_ATOMIC_I
    value = __atomic_load_n(counter, __ATOMIC_ACQUIRE);
    for (;;)
    {
        if (value <= 0)
        {
            return DVM_REFCOUNT_REFUSED;
        }
        if (__atomic_compare_exchange_n(counter, &value, value - 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        {
            break;
        }
    }
#else
    fvm_port_registry_lock();
    value = *counter;
    if (value > 0)
    {
        *counter = value - 1;
    }
    fvm_port_registry_unlock();
    if (value <= 0)
    {
        return DVM_REFCOUNT_REFUSED;
    }
#endif
    return value == 1 ? DVM_REFCOUNT_LAST : DVM_REFCOUNT_LIVE;
}


/* Implementation identity is diagnostic, never a serialized ABI/profile tag. */
const char *dvm_refcount_backend(void)
{
#if defined(DVM_REFCOUNT_ATOMIC_U) && defined(DVM_REFCOUNT_ATOMIC_I)
    return "atomic";
#elif !defined(DVM_REFCOUNT_ATOMIC_U) && !defined(DVM_REFCOUNT_ATOMIC_I)
    return "locked";
#else
    return "mixed";
#endif
}


/* Compare an already-live native state word using the same portable backend. */
int dvm_refcount_compare_i(int *counter, int expected, int desired)
{
#ifdef DVM_REFCOUNT_ATOMIC_I
    return __atomic_compare_exchange_n(counter, &expected, desired, 0,
        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#else
    int matched;
    fvm_port_registry_lock();
    matched = *counter == expected;
    if (matched)
    {
        *counter = desired;
    }
    fvm_port_registry_unlock();
    return matched;
#endif
}
