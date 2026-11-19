/* Copyright (c) 2026 bhgv
 * EN: Contextual /dev/sysevents node; opens capture only the current native owner.
 * RU: Contextual /dev/sysevents node; opens захватывают только current native owner.
 */
#include "sysevents_internal.h"
#include <stdlib.h>
#include <string.h>
static __thread DVM_SysEvents *current; /* Native inbox borrowed from the permanent owner reference. */
static __thread DVM_SysEvents *(*select_current)(void); /* Optional runtime selector; reads only its permanent owner's live frames. */
static DVM_Provider *device; /* Atomic identity of the boot-installed contextual device node. */


/* EN/RU: See internal header; no lookup by PID or foreign process access.
 * См. internal header; без PID lookup и foreign process access. */
DVM_SysEvents *dvm_sysevents_current(void)
{
    return select_current ? select_current() : current;
}


/* EN/RU: See sysevents.h; TLS identity only, no event processing or target cooperation.
 * См. sysevents.h; только TLS identity, без обработки событий/участия target. */
void dvm_sysevents_bind(DVM_SysEvents *events)
{
    current = events;
    select_current = NULL;
}


/* EN/RU: See sysevents.h; only the current native owner changes its TLS selector.
 * См. sysevents.h; TLS-селектор меняет только свой нативный поток.
 */
void dvm_sysevents_select(DVM_SysEvents *(*selector)(void))
{
    select_current = selector;
}


/* EN/RU: Fixed device-node metadata; no source names or provider IDs exposed.
 * Fixed device metadata; без source names/provider IDs. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(output, 0, sizeof(*output));
    strcpy(output->name, "sysevents");
    output->mode = 0444;
    return FVM_OK;
}


/* EN/RU: Capture current native inbox after ordinary namespace resolution.
 * Захватить current inbox после ordinary namespace resolution. */
static FVM_Result open_device(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    (void)state;
    return !strcmp(path, ".") ? dvm_sysevents_open(dvm_sysevents_current(), mode, output) : FVM_NOT_FOUND;
}


/* EN/RU: Contextual device READ is an owner's own inbox capability only.
 * Contextual READ — capability только своего owner inbox. */
static FVM_Result access_check(void *state, const char *path, unsigned access)
{
    (void)state;
    return !strcmp(path, ".") && access == FVM_ACCESS_READ && dvm_sysevents_current() ? FVM_OK : FVM_ERROR;
}


/* EN/RU: Clear atomic device identity after the last bound/open reference retires.
 * Очистить identity после последней bound/open reference. */
static void destroy(void *state)
{
    DVM_Provider *expected = *(DVM_Provider **)state;
    __atomic_compare_exchange_n(&device, &expected, NULL, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
    free(state);
}
static const DVM_ProviderOps ops = {info, open_device, destroy, DVM_PROVIDER_STABLE_BIND, access_check}; /* Stable contextual node, per-open native owner data. */


/* EN/RU: See sysevents.h; caller installs this node through normal VFS binding.
 * См. sysevents.h; caller устанавливает node обычным VFS binding. */
DVM_Provider *dvm_sysevents_device(void)
{
    DVM_Provider **state, *provider;
    if (__atomic_load_n(&device, __ATOMIC_ACQUIRE))
    {
        return NULL;
    }
    state = calloc(1, sizeof(*state));
    if (!state)
    {
        return NULL;
    }
    provider = dvm_provider_create(&ops, state);
    if (!provider)
    {
        free(state);
        return NULL;
    }
    *state = provider;
    DVM_Provider *expected = NULL;
    if (!__atomic_compare_exchange_n(&device, &expected, provider, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
    {
        /* A concurrent constructor won. Destroy only this unpublished candidate;
         * its identity-checked destructor must leave the published device intact.
         * RU: Другой конструктор победил; уничтожить лишь своего кандидата,
         * не сбрасывая опубликованное устройство в его деструкторе. */
        dvm_provider_release(provider);
        return NULL;
    }
    return provider;
}


/* EN/RU: See sysevents.h; only compare identities, never dereference global pointer.
 * См. sysevents.h; только сравнить identities, без dereference global pointer. */
int dvm_sysevents_is_device(DVM_Provider *provider)
{
    return provider == __atomic_load_n(&device, __ATOMIC_ACQUIRE);
}
