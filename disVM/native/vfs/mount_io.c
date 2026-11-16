/* Copyright (c) 2026 bhgv
 * EN: Remote opened-object I/O, directory batches and explicit native cursor ownership.
 * RU: Remote opened-object I/O, directory batches и явное native cursor ownership.
 */
#include "mount_internal.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Query the held remote fid; never reopen the resource's mutable path.
 * Запросить удержанный remote fid, не переоткрывать изменяемый путь. */
static FVM_Result info(void *state, FVM_FileInfo *out)
{
    struct Mounted *opened = state;
    StyxMeta meta;
    if (!styxclientstat(opened->fid, &meta))
    {
        styxmountloss(opened->mount);
        return FVM_ERROR;
    }
    styxmountmeta(opened->mount, &meta, out);
    return FVM_OK;
}


/* EN/RU: One precise remote chunk; SDK dispatcher performs post-failure Tstat once.
 * Один точный remote chunk; SDK dispatcher делает post-failure Tstat один раз. */
static void transfer(void *state, void *bytes, size_t size, int writing, DVM_IoResult *out)
{
    struct Mounted *opened = state;
    StyxTransfer result;
    memset(out, 0, sizeof(*out));
    out->flags = DVM_IO_PREFIX;
    out->status = FVM_ERROR;
    out->stop = DVM_IO_FAILURE;
    if (opened->directory || opened->failed)
    {
        return;
    }
    if (writing)
    {
        styxclientwrite(opened->fid, opened->position, bytes, size, &result);
    }
    else
    {
        styxclientread(opened->fid, opened->position, bytes, size, &result);
    }
    out->count = result.count;
    out->flags = (result.confirmed ? DVM_IO_PREFIX : 0) | (result.uncertain ? DVM_IO_UNCERTAIN : 0);
    out->status = result.success ? FVM_OK : FVM_ERROR;
    out->stop = result.success ? DVM_IO_NONE : result.cancelled ? DVM_IO_CANCELLED : DVM_IO_FAILURE;
    if (result.count <= (unsigned long long)LONG_MAX-opened->position)
    {
        opened->position += result.count;
    }
    else
    {
        out->status = FVM_ERROR;
        out->flags |= DVM_IO_UNCERTAIN;
        opened->failed = 1;
    }
    if (!result.success)
    {
        styxmountloss(opened->mount);
    }
}


/* EN/RU: Read current explicit offset; a short success is not retried automatically.
 * Читать current explicit offset; short success автоматически не повторять. */
static void readchunk(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    transfer(state, bytes, size, 0, out);
}


/* EN/RU: Write exactly one chunk; unknown effects cannot be fabricated as zero success.
 * Записать один chunk; unknown effects не превращаются в zero success. */
static void writechunk(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    transfer(state, (void *)bytes, size, 1, out);
}


/* EN/RU: Seek changes only local positional protocol offset, with checked native-long bounds.
 * Seek меняет только local protocol offset с проверкой native-long bounds. */
static FVM_Result seekfile(void *state, long offset, int origin, long *position)
{
    struct Mounted *opened = state;
    unsigned long long base = origin == FVM_FILE_SEEK_START ? 0 : opened->position;
    if (opened->directory || origin < FVM_FILE_SEEK_START || origin > FVM_FILE_SEEK_END)
    {
        return FVM_ERROR;
    }
    if (origin == FVM_FILE_SEEK_END)
    {
        StyxMeta meta;
        if (!styxclientstat(opened->fid, &meta))
        {
            styxmountloss(opened->mount);
            return FVM_ERROR;
        }
        base = meta.length;
    }
    unsigned long long magnitude = offset < 0 ? (unsigned long long)(-(offset+1))+1 : (unsigned long long)offset;
    if (base > LONG_MAX || (offset < 0 ? magnitude > base : magnitude > (unsigned long long)LONG_MAX-base))
    {
        return FVM_ERROR;
    }
    opened->position = offset < 0 ? base-magnitude : base+magnitude;
    *position = opened->position;
    return FVM_OK;
}


/* EN/RU: Return complete entries from one current directory chunk, at most port batch limit.
 * Вернуть полные entries одного directory chunk, не более port batch limit. */
static FVM_Result list(void *state, FVM_FileInfo **entries, size_t *count)
{
    struct Mounted *opened = state;
    *entries = NULL;
    *count = 0;
    if (!opened->directory || opened->failed)
    {
        return FVM_ERROR;
    }
    FVM_FileInfo *batch = calloc(FVM_FS_BATCH_MAX, sizeof(*batch));
    if (!batch)
    {
        opened->failed = 1;
        return FVM_OUT_OF_MEMORY;
    }
    if (opened->used == opened->size)
    {
        StyxTransfer result;
        styxclientread(opened->fid, opened->position, opened->records, 65536, &result);
        if (!result.success || result.uncertain)
        {
            styxmountloss(opened->mount);
            goto failed;
        }
        opened->position += result.count;
        opened->size = result.count;
        opened->used = 0;
    }
    while (opened->used < opened->size && *count < FVM_FS_BATCH_MAX)
    {
        StyxMeta meta;
        size_t consumed;
        if (!styxclientrecord(opened->records+opened->used, opened->size-opened->used, &meta, &consumed) ||
            !*meta.name || strchr(meta.name, '/') || !strcmp(meta.name, ".") || !strcmp(meta.name, ".."))
        {
            goto failed;
        }
        styxmountmeta(opened->mount, &meta, &batch[*count]);
        (*count)++;
        opened->used += consumed;
    }
    if (*count)
    {
        *entries = batch;
    }
    else
    {
        free(batch);
    }
    return FVM_OK;
failed:
    opened->failed = 1;
    *count = 0;
    free(batch);
    return FVM_ERROR;
}


/* EN/RU: Publication failure invalidates a directory cursor, never turning error into EOF.
 * Ошибка публикации инвалидирует directory cursor, не превращает ошибку в EOF. */
static void abortlist(void *state)
{
    ((struct Mounted *)state)->failed = 1;
}


/* EN/RU: Consume one remote fid and its buffered records, before provider retirement.
 * Поглотить remote fid и records до освобождения provider. */
static void closefile(void *state)
{
    struct Mounted *opened = state;
    styxclientclunk(opened->fid);
    free(opened->records);
    free(opened);
}
const DVM_VfsOps styxmountedops = {info, NULL, NULL, seekfile, list, closefile, abortlist, readchunk, writechunk};
