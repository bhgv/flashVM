/* Copyright (c) 2026 bhgv
 * EN: Keep a stable reverse module root while replacing its caller directories.
 * RU: Сохранять корень встречной публикации при замене каталогов клиентов.
 */
#include "program_view.h"
#include "export.h"
#include "channel_sync.h"
#include <stdlib.h>


/* EN/RU: One current generation, pinned by each in-flight operation.
 * Текущее поколение удерживается каждой начатой операцией. */
struct DVM_ProgramPublication
{
    FVM_PortSync *lock; /* Protects the current generation pointer. */
    DVM_Provider *current; /* Owned complete frozen generation. */
};


/* EN: Retain the current root without running callbacks under the publication lock.
 * RU: Удержать текущий корень без вызова провайдеров под блокировкой публикации. */
static DVM_Provider *snapshot(DVM_ProgramPublication *publication)
{
    fvm_port_sync_lock(publication->lock);
    DVM_Provider *current = publication->current;
    if (!dvm_provider_retain(current))
    {
        current = NULL;
    }
    fvm_port_sync_unlock(publication->lock);
    return current;
}


/* EN: Delegate stat of a borrowed path to one retained generation.
 * RU: Выполнить stat заимствованного пути в одном удерживаемом поколении. */
static FVM_Result publication_info(void *state, const char *path, FVM_FileInfo *out)
{
    DVM_Provider *current = snapshot(state);
    FVM_Result result = current ? dvm_provider_info(current, path, out) : FVM_ERROR;
    dvm_provider_release(current);
    return result;
}


/* EN: Open within a generation. The resulting handle pins its provider.
 * RU: Открыть путь в одном поколении. Дескриптор удерживает его провайдер. */
static FVM_Result publication_open(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    DVM_Provider *current = snapshot(state);
    FVM_Result result = current ? dvm_provider_open(current, path, mode, out) : FVM_ERROR;
    dvm_provider_release(current);
    return result;
}


/* EN: Check effective rights through the same rules as the underlying generation.
 * RU: Проверить фактические права по правилам исходного поколения. */
static FVM_Result publication_access(void *state, const char *path, unsigned access)
{
    DVM_Provider *current = snapshot(state);
    FVM_Result result = current ? dvm_provider_access(current, path, access) : FVM_ERROR;
    dvm_provider_release(current);
    return result;
}


/* EN: Pin the exact node selected from the current generation.
 * RU: Захватить точный узел из текущего поколения. */
static DVM_Provider *publication_capture(void *state, const char *path)
{
    DVM_Provider *current = snapshot(state);
    DVM_Provider *root = current ? dvm_provider_capture(current, path) : NULL;
    dvm_provider_release(current);
    if (!root)
    {
        return NULL;
    }
    FVM_FileInfo info;
    DVM_View *wrapper = dvm_view_create(NULL, NULL, NULL);
    int mapped = wrapper && dvm_provider_info(root, ".", &info) == FVM_OK &&
        dvm_view_map(wrapper, "/", root, ".", FVM_ACCESS_MASK, info.directory ? DVM_VIEW_TREE : 0);
    dvm_provider_release(root);
    if (!mapped)
    {
        dvm_view_discard(wrapper);
        return NULL;
    }
    return dvm_view_freeze(wrapper);
}


/* EN: Consume publication state only after the last root, capture or stream closes.
 * RU: Уничтожить состояние после освобождения корня, захватов и потоков. */
static void publication_destroy(void *state)
{
    DVM_ProgramPublication *publication = state;
    dvm_provider_release(publication->current);
    if (publication->lock)
    {
        fvm_port_sync_destroy(publication->lock);
    }
    free(publication);
}

static const DVM_ProviderOps operations = { /* Stable root with mutable descendants. */
    .info = publication_info, .open = publication_open, .access = publication_access,
    .destroy = publication_destroy
};


/* EN/RU: See program_view.h. Publish an empty root before admitting processes.
 * См. program_view.h. Создать пустой корень до допуска процессов. */
DVM_Provider *dvm_program_publication(DVM_ProgramPublication **control)
{
    if (!control)
    {
        return NULL;
    }
    *control = NULL;
    DVM_ProgramPublication *publication = calloc(1, sizeof(*publication));
    if (!publication)
    {
        return NULL;
    }
    publication->lock = fvm_port_sync_create();
    publication->current = dvm_view_freeze(dvm_view_create(NULL, NULL, NULL));
    DVM_Provider *root = publication->lock && publication->current ?
        dvm_provider_create_capture(&operations, publication, publication_capture, 1) : NULL;
    if (!root)
    {
        publication_destroy(publication);
        return NULL;
    }
    *control = publication;
    return root;
}


/* EN/RU: See program_view.h. Finish construction before switching generations.
 * См. program_view.h. Завершить сборку перед переключением поколения. */
int dvm_program_publication_replace(DVM_ProgramPublication *control,
    const DVM_ProgramView *entries, size_t count)
{
    if (!control)
    {
        return 0;
    }
    DVM_Provider *next = count ? dvm_program_view(entries, count) :
        dvm_view_freeze(dvm_view_create(NULL, NULL, NULL));
    if (!next)
    {
        return 0;
    }
    fvm_port_sync_lock(control->lock);
    DVM_Provider *previous = control->current;
    control->current = next;
    fvm_port_sync_unlock(control->lock);
    dvm_provider_release(previous);
    return 1;
}
