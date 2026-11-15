/* Copyright (c) 2026 bhgv
 * EN: Per-operation export policy gates around ordinary retained provider streams.
 * RU: Per-operation export policy gates вокруг retained provider streams.
 */
#include "view_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Outer provider wrapper pins immutable view until this stream closes.
 * Outer wrapper удерживает immutable view до close stream. */
typedef struct ViewStream
{
    DVM_View *view; /* Borrowed from retaining outer factory wrapper. */
    DVM_VfsHandle inner; /* Owned delegated stream; empty for synthetic directory. */
    char path[DVM_NS_PATH_LIMIT]; /* Copied virtual identity, no source host path. */
    unsigned access; /* Open-time required rights, rechecked before seek. */
    int listed; /* Explicit directory overlay names already emitted. */
    int failed; /* List publication failure forbids silently skipping a batch. */
} ViewStream;


/* EN/RU: Recheck current virtual policy before any inner operation.
 * Перепроверить current virtual policy до inner operation;1/0. */
static int allowed(ViewStream *stream, unsigned access)
{
    DVM_ViewRule *rule;
    char relative[DVM_NS_PATH_LIMIT];
    return dvm_view_route(stream->view, stream->path, access, &rule, relative) != 0;
}


/* EN/RU: Current policy-gated metadata; no implicit authority from old FD mode.
 * Current metadata через policy gate; old FD mode не даёт authority. */
static FVM_Result info(void *state, FVM_FileInfo *output)
{
    ViewStream *stream = state;
    return dvm_view_info(stream->view, stream->path, output);
}


/* EN/RU: Source hints use different journal/resource identities; automatic source
 * forwarding already publishes safe provider-wide invalidations. Mask diagnostic
 * modes without repeating the source query, preserving original acknowledgement.
 * Source hints принадлежат другому journal; очистить их, mode ограничить view policy,
 * сохранив count/status и не повторяя source query. */
static void scope_result(ViewStream *stream, DVM_IoResult *result)
{
    result->flags &= ~DVM_IO_CHANGED;
    result->session = result->generation = result->resource = 0;
    result->change_kinds = result->change_flags = 0;
    if (result->flags & DVM_IO_MODE)
    {
        DVM_ViewRule *rule;
        char relative[DVM_NS_PATH_LIMIT];
        int route = dvm_view_route(stream->view, stream->path, 0, &rule, relative);
        if (!route)
        {
            result->flags &= ~DVM_IO_MODE;
            result->observed_mode = 0;
            result->probe_status = FVM_UNSUPPORTED;
        }
        else
        {
            unsigned mask = route == 2 ? FVM_ACCESS_READ | FVM_ACCESS_EXEC : rule->access;
            for (unsigned bit = 1; bit <= FVM_ACCESS_READ; bit <<= 1)
            {
                if (stream->view->policy && !stream->view->policy(stream->view->context, stream->path, bit))
                {
                    mask &= ~bit;
                }
            }
            result->observed_mode &= ~0777UL | (mask << 6) | (mask << 3) | mask;
        }
    }
}


/* EN/RU: Preserve real chunk acknowledgement; denied policy performs no source I/O.
 * Сохранить real chunk result; denied policy не вызывает source I/O. */
static void read_chunk(void *state, void *bytes, size_t size, DVM_IoResult *result)
{
    ViewStream *stream = state;
    if (allowed(stream, FVM_ACCESS_READ))
    {
        *result = dvm_vfs_read_chunk(&stream->inner, bytes, size);
        scope_result(stream, result);
    }
    else
    {
        memset(result, 0, sizeof(*result));
        result->flags = DVM_IO_PREFIX;
        result->status = FVM_ERROR;
        result->stop = DVM_IO_DENIED;
    }
}


/* EN/RU: Forward immutable bytes once after current write policy; no replay.
 * Передать bytes один раз после current write policy; без replay. */
static void write_chunk(void *state, const void *bytes, size_t size, DVM_IoResult *result)
{
    ViewStream *stream = state;
    if (allowed(stream, FVM_ACCESS_WRITE))
    {
        *result = dvm_vfs_write_chunk(&stream->inner, bytes, size);
        scope_result(stream, result);
    }
    else
    {
        memset(result, 0, sizeof(*result));
        result->flags = DVM_IO_PREFIX;
        result->status = FVM_ERROR;
        result->stop = DVM_IO_DENIED;
    }
}


/* EN/RU: Seek only while current rights still permit the opened use.
 * Seek разрешён только пока current rights допускают opened use. */
static FVM_Result seek(void *state, long offset, int origin, long *position)
{
    ViewStream *stream = state;
    FVM_Result status;
    if (!allowed(stream, stream->access))
    {
        return FVM_ERROR;
    }
    if (stream->inner.ops)
    {
        status = dvm_vfs_seek(&stream->inner, offset, origin, position);
    }
    else if (!offset && origin == FVM_FILE_SEEK_START && position)
    {
        *position = 0;
        status = FVM_OK;
    }
    else
    {
        status = FVM_UNSUPPORTED;
    }
    if (status == FVM_OK)
    {
        stream->listed = 0;
        stream->failed = 0;
    }
    return status;
}


/* EN/RU: Return filtered/overlaid directory entries, preserving one cursor.
 * Вернуть filtered/overlaid directory entries, сохранив один cursor. */
static FVM_Result list(void *state, FVM_FileInfo **entries, size_t *count)
{
    ViewStream *stream = state;
    if (stream->failed || !allowed(stream, FVM_ACCESS_READ | FVM_ACCESS_EXEC))
    {
        return FVM_ERROR;
    }
    FVM_Result status = dvm_view_list(stream->view, stream->path, &stream->inner, stream->listed, entries, count);
    if (status == FVM_OK)
    {
        stream->listed = 1;
    }
    else
    {
        stream->failed = 1;
    }
    return status;
}


/* EN/RU: Invalidate enumeration after managed batch publication failure.
 * Инвалидировать enumeration после managed batch publication failure. */
static void abort_list(void *state)
{
    ((ViewStream *)state)->failed = 1;
}


/* EN/RU: Consume inner stream then native staging; outer wrapper releases factory.
 * Закрыть inner stream и native staging; outer wrapper освободит factory. */
static void close_stream(void *state)
{
    ViewStream *stream = state;
    dvm_vfs_close(&stream->inner);
    free(stream);
}
static const DVM_VfsOps ops = {info, NULL, NULL, seek, list, close_stream, abort_list, read_chunk, write_chunk}; /* Policy-enforced ordinary I/O. */


/* EN/RU: See internal header; no native stream is exposed before all checks succeed.
 * См. internal header; stream публикуется только после всех checks. */
FVM_Result dvm_view_open(DVM_View *view, const char *path, const char *mode, DVM_VfsHandle *output)
{
    DVM_ViewRule *rule;
    char relative[DVM_NS_PATH_LIMIT];
    FVM_FileInfo metadata;
    unsigned access = !strcmp(mode, "r") ? FVM_ACCESS_READ : !strcmp(mode, "rw") ?
        FVM_ACCESS_READ | FVM_ACCESS_WRITE :
        (!strcmp(mode, "wo") || !strcmp(mode, "w") || !strcmp(mode, "wx")) ? FVM_ACCESS_WRITE : 0;
    if (!access)
    {
        return FVM_UNSUPPORTED;
    }
    FVM_Result status = dvm_view_info(view, path, &metadata);
    if (status == FVM_OK && metadata.directory)
    {
        access |= FVM_ACCESS_EXEC;
    }
    int virtual_parent = status == FVM_OK && metadata.directory && dvm_view_children(view, path);
    int route = dvm_view_route(view, path, access, &rule, relative);
    if (!route)
    {
        return FVM_ERROR;
    }
    ViewStream *stream = calloc(1, sizeof(*stream));
    if (!stream)
    {
        return FVM_OUT_OF_MEMORY;
    }
    stream->view = view;
    stream->access = access;
    strcpy(stream->path, path);
    if (route == 1)
    {
        status = dvm_provider_open(rule->source, relative, mode, &stream->inner);
        if (status != FVM_OK)
        {
            if (!virtual_parent)
            {
                free(stream);
                return status;
            }
            dvm_vfs_close(&stream->inner);
        }
    }
    output->ops = &ops;
    output->state = stream;
    return FVM_OK;
}
