/* Atomic native GC scalar fields with portable registry-lock fallback. */
#include "gc_fields.h"
#include "channel_sync.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(DVM_GC_FIELDS_FORCE_LOCKED) && defined(__GCC_ATOMIC_INT_LOCK_FREE) && __GCC_ATOMIC_INT_LOCK_FREE == 2
#define GC_FIELDS_ATOMIC 1 /* Native int atomics; otherwise all operations use registry mutex. */
#endif


/* Geometry only: caller independently guarantees mapped storage/lifetime. */
static int aligned(const void *address)
{
    return address && (uintptr_t)address%sizeof(int) == 0;
}


/* Caller holds fallback registry exclusion; native implementation needs none. */
static int read_field(const int *source)
{
#ifdef GC_FIELDS_ATOMIC
    return __atomic_load_n(source, __ATOMIC_ACQUIRE);
#else
    return *source;
#endif
}


/* Release publishes this scalar, never the whole graph/collector epoch. */
static void write_field(int *destination, int value)
{
#ifdef GC_FIELDS_ATOMIC
    __atomic_store_n(destination, value, __ATOMIC_RELEASE);
#else
    *destination = value;
#endif
}


/* Read only protected source storage; result is private nonshared output. */
int dvm_gc_field_load(const int *source, int *result)
{
    int value;
    if (!aligned(source) || !aligned(result) || source == result)
    {
        return 0;
    }
#ifndef GC_FIELDS_ATOMIC
    fvm_port_registry_lock();
#endif
    value = read_field(source);
#ifndef GC_FIELDS_ATOMIC
    fvm_port_registry_unlock();
#endif
    *result = value;
    return 1;
}


/* Write one metadata cell without allocation or error callbacks. */
int dvm_gc_field_store(int *destination, int value)
{
    if (!aligned(destination))
    {
        return 0;
    }
#ifndef GC_FIELDS_ATOMIC
    fvm_port_registry_lock();
#endif
    write_field(destination, value);
#ifndef GC_FIELDS_ATOMIC
    fvm_port_registry_unlock();
#endif
    return 1;
}


/* Atomically replace one scalar and return its predecessor in private storage.
 * Native ACQ_REL exchange and locked fallback have one linearization point.
 * Caller supplies live cells and must not hold the fallback registry lock. */
int dvm_gc_field_exchange(int *destination, int value, int *previous)
{
    int observed;
    if (!aligned(destination) || !aligned(previous) || destination == previous)
    {
        return 0;
    }
#ifdef GC_FIELDS_ATOMIC
    observed = __atomic_exchange_n(destination, value, __ATOMIC_ACQ_REL);
#else
    fvm_port_registry_lock();
    observed = read_field(destination);
    write_field(destination, value);
    fvm_port_registry_unlock();
#endif
    *previous = observed;
    return 1;
}


/* Preserve Setmark's conditional color/dirty behavior. Under native atomics,
 * two stores are intentionally NOT advertised as one epoch/graph transaction. */
int dvm_gc_field_mark(int *color, const int *selector, int *pending)
{
    int selected;
    int previous;
    int valid;
    if (!aligned(color) || !aligned(selector) || !aligned(pending) ||
        color == selector || color == pending || selector == pending)
    {
        return 0;
    }
#ifndef GC_FIELDS_ATOMIC
    fvm_port_registry_lock();
#endif
    selected = read_field(selector);
    previous = read_field(color);
    valid = selected >= 0 && selected <= 2 && previous >= 0 && previous <= 3;
    if (valid && previous != selected)
    {
        write_field(color, 3);
        write_field(pending, 1);
    }
#ifndef GC_FIELDS_ATOMIC
    fvm_port_registry_unlock();
#endif
    return valid;
}


/* Fatal host invariant only after native access has released all locks. */
static void invalid(void)
{
    fprintf(stderr, "disvm GC metadata invariant\n");
    abort();
}


/* [GC:] Observe protected native metadata; no source lifetime acquisition. */
int dvm_runtime_gc_load(const int *source)
{
    int value;
    if (!dvm_gc_field_load(source, &value))
    {
        invalid();
    }
    return value;
}


/* [GC:] Write protected native metadata under existing managed/collector role. */
void dvm_runtime_gc_store(int *destination, int value)
{
    if (!dvm_gc_field_store(destination, value))
    {
        invalid();
    }
}


/* [GC:] Mark only a protected live Heap color; selector/epoch exclusion remains. */
void dvm_runtime_gc_mark(int *color, const int *selector, int *pending)
{
    if (!dvm_gc_field_mark(color, selector, pending))
    {
        invalid();
    }
}


/* [GC:] Consume scalar work notification under existing collector exclusion.
 * Return its old value; terminate on malformed geometry after releasing locks.
 * This acquires no source lifetime or graph/epoch-completion permission. */
int dvm_runtime_gc_exchange(int *destination, int value)
{
    int previous;
    if (!dvm_gc_field_exchange(destination, value, &previous))
    {
        invalid();
    }
    return previous;
}
