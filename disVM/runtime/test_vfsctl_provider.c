/* Copyright (c) 2026 bhgv
 * EN: Trusted private provider for real Vfsctl guest mixed-stack/property tests.
 * RU: Private trusted provider для real Vfsctl mixed-stack/property guest tests.
 */
#include "vfs/boot.h"
#include "vfs/resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int state; /* Fixed native factory state, lives through runtime shutdown. */
static unsigned destroyed; /* Final provider factory destruction count. */
int __real_dvm_namespace_boot_initialize(const char *directory); /* Unchanged production startup. */
void __real_dvm_close_worker_stop(void); /* Join ordinary production FD retirement. */


/* EN/RU: Fail explicitly on a false native test contract.
 * Явный failure при нарушении native test contract. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "Vfsctl provider: %s\n", label);
        abort();
    }
}


/* EN/RU: Return copied fixed root/file metadata; paths are capability-relative.
 * Вернуть copied metadata root/file; paths capability-relative. */
static FVM_Result info(void *data, const char *path, FVM_FileInfo *output)
{
    (void)data;
    if (strcmp(path, ".") && strcmp(path, "f"))
    {
        return FVM_NOT_FOUND;
    }
    memset(output, 0, sizeof(*output));
    output->directory = !strcmp(path, ".");
    output->mode = output->directory ? 0555 : 0444;
    return FVM_OK;
}


/* EN/RU: Effective READ only; declaration is not inferred from metadata mode.
 * Разрешить только effective READ, независимо от metadata mode. */
static FVM_Result access_check(void *data, const char *path, unsigned access)
{
    (void)data;
    return !strcmp(path, "f") && access == FVM_ACCESS_READ ? FVM_OK : FVM_ERROR;
}


/* EN/RU: Read one native immutable byte with a confirmed prefix.
 * Прочитать один native immutable byte с confirmed prefix. */
static FVM_Result read_file(void *data, void *bytes, size_t size, size_t *received)
{
    (void)data;
    *received = size ? 1 : 0;
    if (size)
    {
        *(char *)bytes = 'P';
    }
    return FVM_OK;
}


/* EN/RU: Native file state is borrowed from the retaining factory wrapper.
 * Native file state заимствован у retaining factory wrapper; no allocation. */
static void close_file(void *data)
{
    (void)data;
}
static const DVM_VfsOps file_ops = {NULL, read_file, NULL, NULL, NULL, close_file}; /* Readonly source byte. */


/* EN/RU: Open one fixed file; factory wrapper pins state through final close.
 * Открыть fixed file; wrapper удерживает state до final close. */
static FVM_Result open_file(void *data, const char *path, const char *mode, DVM_VfsHandle *output)
{
    if (strcmp(path, "f") || strcmp(mode, "r"))
    {
        return FVM_UNSUPPORTED;
    }
    output->ops = &file_ops;
    output->state = data;
    return FVM_OK;
}


/* EN/RU: Record final factory release after bindings/FDs/subscriptions are gone.
 * Посчитать final release после bindings/FDs/subscriptions. */
static void destroy(void *data)
{
    (void)data;
    destroyed++;
}
static const DVM_ProviderOps ops = {info, open_file, destroy, DVM_PROVIDER_STABLE_BIND, access_check}; /* Fixed node identities. */


/* EN/RU: Install private source by ordinary host mount, without a global guest registry.
 * Установить private source обычным host mount; без global guest registry. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    DVM_NamespacePlan target;
    DVM_Provider *provider;
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    provider = dvm_provider_create(&ops, &state);
    require(provider && dvm_namespace_plan(dvm_namespace_boot_group(), "/provider", &target) &&
            dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, provider, "/private-provider") == 1,
            "private source mount");
    dvm_provider_release(provider);
    return 1;
}


/* EN/RU: Verify actual native lifetime after normal production close-worker join.
 * Проверить actual lifetime после normal production close-worker join. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    require(destroyed == 1, "binding/FD/subscription lifetime leak");
    puts("PASS Vfsctl provider lifetime");
}
