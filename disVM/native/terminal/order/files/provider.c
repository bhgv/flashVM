/* Copyright (c) 2026 bhgv
 * EN: Single-file VFS grants with ordinary rw attributes and no ambient transport/root authority.
 * RU: VFS grants одного файла с обычными rw-атрибутами без ambient transport/root authority.
 */
#include "internal.h"


/* EN/RU: See internal.h; only dot exists in this single-file publication, not a directory tree.
 * См. internal.h; в публикации одного файла существует лишь dot, не дерево каталогов.
 */
FVM_Result orderedfileinfo(void *state, const char *path, FVM_FileInfo *output)
{
    OrderedFile *file = state; /* Factory retained by caller/provider wrapper. */
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(output, 0, sizeof(*output));
    strcpy(output->name, "records");
    output->mode = ((file->access & FVM_ACCESS_READ) ? 0444 : 0) | ((file->access & FVM_ACCESS_WRITE) ? 0222 : 0);
    output->identity.low = file->tag;
    output->device.low = 0x554e4942UL;
    return FVM_OK;
}


/* EN/RU: Open one exclusive cursor with copied record buffers; reject unsupported/truncating modes before allocation.
 * Открыть свой cursor с buffers записей; отвергнуть неподдерживаемые/truncating modes до allocation.
 */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    OrderedFile *file = state; /* Ordinary wrapper retains factory for the entire opened handle. */
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    unsigned access = !strcmp(mode, "r") ? FVM_ACCESS_READ : !strcmp(mode, "wo") ? FVM_ACCESS_WRITE :
        !strcmp(mode, "rw") ? FVM_ACCESS_READ | FVM_ACCESS_WRITE : 0; /* wo is nontruncating native write. */
    if (!access || (access & file->access) != access)
    {
        return FVM_ERROR;
    }
    OrderedFileOpen *opened = calloc(1, sizeof(*opened)); /* Private until both buffers exist. */
    if (opened)
    {
        opened->payload = malloc(file->ceiling + 1);
        opened->encoded = malloc(file->ceiling + 32);
    }
    if (!opened || !opened->payload || !opened->encoded)
    {
        if (opened)
        {
            free(opened->payload);
            free(opened->encoded);
        }
        free(opened);
        return FVM_OUT_OF_MEMORY;
    }
    opened->file = file;
    opened->access = access;
    output->state = opened;
    output->ops = &orderedfileops;
    return FVM_OK;
}


/* EN/RU: Last factory/open pin releases only native counters/storage; never joins or stops the shared broker.
 * Последний factory/open pin освобождает лишь counters/storage; не выполняет join/stop общего broker.
 */
static void destroy(void *state)
{
    OrderedFile *file = state; /* Exclusive final factory owner. */
    pthread_mutex_lock(&file->files->lock);
    file->files->factories--;
    pthread_mutex_unlock(&file->files->lock);
    free(file);
}
static const DVM_ProviderOps operations = {.info = orderedfileinfo, .open = openfile, .destroy = destroy,
    .flags = DVM_PROVIDER_STABLE_BIND}; /* Dot identity is stable, contents remain a live stream. */


/* EN/RU: See files.h; increment owner pin before provider publication and undo only staged state on refusal.
 * См. files.h; увеличить owner pin до публикации provider, при отказе откатить лишь staged-state.
 */
DVM_Provider *styxorderedfile(StyxOrderedFiles *files, unsigned long tag, unsigned access)
{
    if (!files || !access || (access & ~(FVM_ACCESS_READ | FVM_ACCESS_WRITE)))
    {
        return NULL;
    }
    StyxOrderedClient *client = styxorderedclient(files->peer, tag); /* Never infer grants from arbitrary path names. */
    OrderedFile *file = client ? calloc(1, sizeof(*file)) : NULL; /* Private filtered capability. */
    if (!file)
    {
        return NULL;
    }
    file->files = files;
    file->client = client;
    file->tag = tag;
    file->access = access;
    file->ceiling = styxorderedclientceiling(client);
    pthread_mutex_lock(&files->lock);
    int full = files->factories == (size_t)-1; /* Refuse count overflow without publication. */
    if (!full)
    {
        files->factories++;
    }
    pthread_mutex_unlock(&files->lock);
    if (full)
    {
        free(file);
        return NULL;
    }
    DVM_Provider *provider = dvm_provider_create_native(&operations, file, NULL, 1, DVM_PROVIDER_CANCEL_PATH); /* Owns file only on success. */
    if (!provider)
    {
        destroy(file);
    }
    return provider;
}
