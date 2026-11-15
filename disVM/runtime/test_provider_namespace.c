/* Copyright (c) 2026 bhgv
 * EN: Mount a real retained namespace provider at startup; no per-FD substitution.
 * RU: Монтировать retained namespace provider при startup; без per-FD подмены.
 */
#define DVM_PROVIDER_NAMESPACE 1 /* Omit the older per-FD injection boundary. */
#define DVM_PROVIDER_EXPECTED 30 /* Eight read/write/list sets plus retained and alias FDs. */
#define __wrap_dvm_close_worker_stop provider_base_stop
#include "test_provider_fd.c"
#undef __wrap_dvm_close_worker_stop
#include "vfs/boot.h"
#include "vfs/resource.h"
static unsigned destroyed; /* Factory destroy count, observed after worker join. */
int __real_dvm_namespace_boot_initialize(const char *);


/* EN/RU: Validate provider-relative names and report copied native metadata.
 * Проверить provider-relative names и вернуть copied native metadata. */
static FVM_Result factory_info(void *state, const char *relative, FVM_FileInfo *output)
{
    Memory value = {0};
    if (*(int *)state != 1)
    {
        abort();
    }
    if (!strcmp(relative, "."))
    {
        value.directory = 1;
    }
    else if (!strcmp(relative, "provider-read"))
    {
        value.writable = 0;
    }
    else if (!strcmp(relative, "provider-write"))
    {
        value.writable = 1;
    }
    else if (!strcmp(relative, "provider-list"))
    {
        value.directory = 1;
    }
    else
    {
        return FVM_ERROR;
    }
    return info(&value, output);
}


/* EN/RU: Open only declared read/write modes into independently owned Memory.
 * Missing/mismatched modes fail empty; allocation precedes all state changes.
 * Открыть только declared modes в свой Memory; отказ оставляет output пустым,
 * allocation предшествует изменениям. */
static FVM_Result factory_open(void *state, const char *relative, const char *mode, DVM_VfsHandle *output)
{
    Memory *memory;
    int writable = !strcmp(relative, "provider-write");
    int directory = !strcmp(relative, "provider-list");
    if (*(int *)state != 1)
    {
        abort();
    }
    if ((!writable && !directory && strcmp(relative, "provider-read")) ||
        strcmp(mode, writable ? "wo" : "r"))
    {
        return FVM_UNSUPPORTED;
    }
    memory = calloc(1, sizeof(*memory));
    if (!memory)
    {
        return FVM_OUT_OF_MEMORY;
    }
    memcpy(memory->bytes, "ABCDE", sizeof(memory->bytes));
    memory->writable = writable;
    memory->directory = directory;
    output->ops = directory ? &list_ops : writable ? &write_ops : &read_ops;
    output->state = memory;
    pthread_mutex_lock(&sync);
    created++;
    pthread_mutex_unlock(&sync);
    return FVM_OK;
}


/* EN/RU: Last factory reference must follow every real consuming handle close.
 * Последняя factory reference должна следовать всем consuming handle closes. */
static void factory_destroy(void *state)
{
    if (created != DVM_PROVIDER_EXPECTED || closed != created)
    {
        abort();
    }
    *(int *)state = 0;
    free(state);
    destroyed++;
}
static const DVM_ProviderOps factory_ops = {factory_info, factory_open, factory_destroy, DVM_PROVIDER_STABLE_BIND}; /* Immutable trusted host factory. */


/* EN/RU: Initialize real startup group, then mount one provider at virtual view.
 * Transfer references through the production host API, not through open hooks.
 * Инициализировать настоящую группу и смонтировать provider на view через host
 * API; open hooks не используются. Startup-only; failure terminates test. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    DVM_NamespacePlan target;
    DVM_Provider *provider;
    int *state;
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    state = malloc(sizeof(*state));
    if (!state)
    {
        abort();
    }
    *state = 1;
    provider = dvm_provider_create(&factory_ops, state);
    if (!provider || !dvm_namespace_plan(dvm_namespace_boot_group(), "view", &target) ||
        dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, provider, "/memory-provider") != 1)
    {
        abort();
    }
    dvm_provider_release(provider);
    return 1;
}


/* EN/RU: Join real worker and verify factory cleanup after the last opened FD.
 * Дождаться worker и проверить cleanup factory после последнего opened FD. */
void __wrap_dvm_close_worker_stop(void)
{
    provider_base_stop();
    if (destroyed != 1)
    {
        fprintf(stderr, "FAIL provider factory destroy count=%u\n", destroyed);
        exit(9);
    }
    puts("PASS mounted provider destroyed once after all opened handles");
}
