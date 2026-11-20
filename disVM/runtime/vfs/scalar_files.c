/* Copyright (c) 2026 bhgv
 * EN: Native lifetime for an exact request/reply grant.
 * RU: Нативное владение точными файлами запроса и ответа.
 */
#include "scalar_files.h"
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include <limits.h>
#include <stdlib.h>


/* EN/RU: Consume route storage and its two file owners.
 * Освободить маршрут и обе его ссылки на файлы. */
static void destroy_files(void *state)
{
    DVM_ScalarFiles *files = state;
    dvm_provider_release(files->request);
    dvm_provider_release(files->reply);
    free(files);
}

static const DVM_VfsOps cleanup = {.close = destroy_files}; /* Native closer callback. */


/* EN/RU: Capture both grants before returning a published route.
 * Захватить оба файла до публикации маршрута. */
DVM_ScalarFiles *dvm_scalar_files(DVM_Provider *request, DVM_Provider *reply)
{
    if (!request || !reply || request == reply)
    {
        return NULL;
    }
    DVM_ScalarFiles *files = calloc(1, sizeof(*files));
    if (!files)
    {
        return NULL;
    }
    files->request = dvm_provider_capture(request, ".");
    files->reply = dvm_provider_capture(reply, ".");
    files->close = dvm_close_prepare();
    if (!files->close || !(dvm_provider_native_flags(files->request) & DVM_PROVIDER_CANCEL_PATH) ||
        !(dvm_provider_native_flags(files->reply) & DVM_PROVIDER_CANCEL_PATH))
    {
        dvm_close_discard(files->close);
        destroy_files(files);
        return NULL;
    }
    files->references = 1;
    return files;
}


/* EN/RU: Retain without allowing overflow.
 * Удержать ссылку без переполнения счётчика. */
int dvm_scalar_files_retain(DVM_ScalarFiles *files)
{
    if (!files)
    {
        return 1;
    }
    unsigned current = __atomic_load_n(&files->references, __ATOMIC_RELAXED);
    while (current && current < UINT_MAX)
    {
        if (__atomic_compare_exchange_n(&files->references, &current, current + 1, 1,
                __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: Queue final provider release on the native closer.
 * Поставить окончательное освобождение провайдеров в очередь нативного closer. */
void dvm_scalar_files_release(DVM_ScalarFiles *files)
{
    if (files && __atomic_sub_fetch(&files->references, 1, __ATOMIC_ACQ_REL) == 0)
    {
        DVM_VfsHandle handle = {.ops = &cleanup, .state = files};
        dvm_close_submit_resource(files->close, &handle);
    }
}


/* EN/RU: Detach the native route before retiring the module descriptor.
 * Отсоединить нативный маршрут перед освобождением дескриптора модуля. */
void dvm_scalar_module_retire(Module *module)
{
    DVM_ScalarFiles *files = module->scalar_files;
    module->scalar_files = NULL;
    dvm_scalar_files_release(files);
}
