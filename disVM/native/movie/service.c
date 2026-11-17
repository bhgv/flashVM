/* Copyright (c) 2026 bhgv
 * EN: Per-open S-record acquisition streams with bounded memory and native resource ownership.
 * RU: Per-open S-record streams загрузки с bounded memory и native resource ownership.
 */
#include "service_internal.h"


/* EN/RU: Describe only the granted service file; borrowed output, no resource-path lookup.
 * Описать только granted service file; borrowed output, без resource-path lookup. */
static FVM_Result metadata(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    if (!path || strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, "movie");
    out->identity.low = 1;
    out->mode = 0666;
    return FVM_OK;
}


/* EN/RU: Query borrowed open metadata, never the currently requested resource's identity.
 * Получить metadata borrowed open, не identity запрошенного ресурса. */
static FVM_Result openinfo(void *state, FVM_FileInfo *out)
{
    return metadata(state, ".", out);
}


/* EN/RU: Consume exclusive open and any held source; pending bytes are explicitly discarded.
 * Поглотить exclusive open/source; pending bytes явно отбрасываются. */
static void closeopen(void *state)
{
    MovieOpen *opened = state;
    dvm_vfs_close(&opened->resource);
    free(opened);
}


/* EN/RU: Accept partial request bytes; malformed/busy streams fail permanently without replay.
 * Заимствовать bytes до return; malformed/busy stream получает sticky failure без replay. */
static FVM_Result writeopen(void *state, const void *bytes, size_t size)
{
    MovieOpen *opened = state;
    FVM_SRecord record;
    size_t consumed;
    FVM_SResult status;
    if (opened->failed || opened->requested || size > sizeof(opened->input)-opened->used)
    {
        opened->failed = 1;
        return FVM_ERROR;
    }
    if (!size)
    {
        return FVM_OK;
    }
    memcpy(opened->input+opened->used, bytes, size);
    opened->used += size;
    while (opened->used)
    {
        status = fvm_srecord_read(opened->input, opened->used, MOVIE_FRAME, &record, &consumed);
        if (status == FVM_S_MORE)
        {
            return FVM_OK;
        }
        if (status != FVM_S_OK || opened->requested)
        {
            opened->failed = 1;
            return FVM_ERROR;
        }
        if (record.tag == FVM_SRECORD_CONTROL_TAG)
        {
            status = fvm_sbindings_apply(&opened->incoming, opened->input, consumed, &consumed);
            if (status != FVM_S_OK || !movie_bound(&opened->incoming))
            {
                opened->failed = 1;
            }
        }
        else if (record.tag != MOVIE_TAG || !movie_bound(&opened->incoming) ||
            !movie_request(opened, record.payload, record.size))
        {
            opened->failed = 1;
        }
        if (record.tag == MOVIE_TAG)
        {
            opened->requested = 1;
        }
        if (opened->failed)
        {
            return FVM_ERROR;
        }
        opened->used -= consumed;
        memmove(opened->input, opened->input+consumed, opened->used);
    }
    return FVM_OK;
}


/* EN/RU: Generate one bounded response; source errors retain only acknowledged byte prefixes.
 * Создать один bounded response; ошибки source сохраняют только подтверждённый prefix. */
static int nextrecord(MovieOpen *opened)
{
    unsigned char bytes[MOVIE_CHUNK];
    if (opened->stage == MOVIE_BEGIN)
    {
        opened->stage = MOVIE_BYTES;
        return movie_record(opened->output, &opened->size, MOVIE_BEGIN,
            opened->origin, strlen(opened->origin));
    }
    if (opened->stage == MOVIE_BYTES)
    {
        DVM_IoResult result = dvm_vfs_read_chunk(&opened->resource, bytes, sizeof(bytes));
        if (result.status != FVM_OK ||
            (result.stop != DVM_IO_NONE && result.stop != DVM_IO_EOF) || !(result.flags & DVM_IO_PREFIX) ||
            (result.flags & DVM_IO_UNCERTAIN) || result.count > sizeof(bytes))
        {
            opened->outcome = 1;
            opened->stage = MOVIE_END;
        }
        if ((result.flags & DVM_IO_PREFIX) && !(result.flags & DVM_IO_UNCERTAIN) &&
            result.count && result.count <= sizeof(bytes))
        {
            return movie_record(opened->output, &opened->size, MOVIE_BYTES, bytes, result.count);
        }
        opened->stage = MOVIE_END;
    }
    if (opened->stage == MOVIE_END)
    {
        unsigned char outcome = (unsigned char)opened->outcome;
        dvm_vfs_close(&opened->resource);
        opened->stage = 0;
        return movie_record(opened->output, &opened->size, MOVIE_END, &outcome, 1);
    }
    return 0;
}


/* EN/RU: Return at most the current record prefix;0 only after terminal response was consumed.
 * Borrow output through return, native source I/O may block; no VM entry or guest pause.
 * Вернуть prefix текущего record;0 после terminal response, без VM entry/guest pause. */
static FVM_Result readopen(void *state, void *bytes, size_t capacity, size_t *received)
{
    MovieOpen *opened = state;
    *received = 0;
    if (opened->failed)
    {
        return FVM_ERROR;
    }
    if (!capacity)
    {
        return FVM_OK;
    }
    if (opened->sent == opened->size)
    {
        opened->sent = opened->size = 0;
        if (!nextrecord(opened))
        {
            return FVM_OK;
        }
    }
    size_t count = opened->size-opened->sent;
    if (count > capacity)
    {
        count = capacity;
    }
    memcpy(bytes, opened->output+opened->sent, count);
    opened->sent += count;
    *received = count;
    return FVM_OK;
}


static const DVM_VfsOps operations = {
    openinfo, readopen, writeopen, NULL, NULL, closeopen, NULL, NULL, NULL
}; /* Immutable native per-open dispatch; no seek, directory or managed callbacks. */


/* EN/RU: Open independent duplex session; borrowed namespace pinned by provider wrapper.
 * Output adopted only on success; mode rw is required, no creation/truncation.
 * Открыть независимую duplex session; output принят только при успехе, mode rw обязателен. */
static FVM_Result openservice(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    if (strcmp(path, ".") || strcmp(mode, "rw"))
    {
        return FVM_UNSUPPORTED;
    }
    MovieOpen *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return FVM_OUT_OF_MEMORY;
    }
    opened->group = state;
    fvm_sbindings_init(&opened->incoming, &opened->slot, 1);
    if (!movie_binding(opened->output, &opened->size))
    {
        free(opened);
        return FVM_ERROR;
    }
    out->ops = &operations;
    out->state = opened;
    return FVM_OK;
}


/* EN/RU: Drop the service's namespace pin after all independently opened sessions retire.
 * Освободить namespace pin после завершения всех независимо открытых sessions. */
static void destroy(void *state)
{
    dvm_namespace_release(state);
}


static const DVM_ProviderOps factory = {
    metadata, openservice, destroy, 0, NULL
}; /* Endpoint authority comes from the host's prepared namespace. */


/* EN/RU: See service.h; publish no partial ownership on failure.
 * См. service.h; при отказе не публиковать partial ownership. */
DVM_Provider *styxmovieservice(DVM_Namespace *group)
{
    if (!group || !dvm_namespace_retain(group))
    {
        return NULL;
    }
    DVM_Provider *provider = dvm_provider_create_capture(&factory, group, NULL, 1);
    if (!provider)
    {
        dvm_namespace_release(group);
    }
    return provider;
}
