/* Native synthetic resource files. All calls are externally serialized; VM work stays in pump. */
#include "flashvm/rfs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* One fixed open slot, with private observer/response state and a coherent directory snapshot. */
typedef struct RFile
{
    unsigned long generation; /* Retired on Uint32 exhaustion. */
    int active;               /* This incarnation is open. */
    FVM_RFSNode node;         /* Resource incarnation and file kind. */
    FVM_RWatch watch;         /* Independent events subscription, zero for other kinds. */
    unsigned long request;    /* Posted request awaiting take, zero when absent. */
    int written;              /* A CALL permits exactly one write, including an empty request. */
    int declaration;          /* The file-local tag declaration has already been prepared. */
    unsigned long offset;     /* Exact next read offset for binary streams. */
    size_t start;             /* Consumed prefix of the retained encoded record. */
    size_t size;              /* Prepared output extent. */
    unsigned char bytes[FVM_RFS_BUFFER]; /* Retained native copy, no borrowed packet memory. */
    size_t count;                        /* Number of visible resources at directory open. */
    FVM_RResource rows[FVM_R_RESOURCES]; /* Immutable directory discovery snapshot. */
} RFile;
/* One authenticated connection, not an authority-granting mount. */
struct FVM_RFS
{
    FVM_RDispatch *dispatch;    /* Borrowed through destroy; owns its own lock policy. */
    unsigned long session;      /* Host-authenticated identity, exclusive request-ID namespace. */
    unsigned long next;         /* Last successfully posted request ID; never wraps. */
    RFile files[FVM_RFS_FILES]; /* Fixed storage, allocated once at create. */
};
/* Reject native wide values outside the portable wire identity domain. */
static int fs_id(unsigned long value)
{
    return value && value <= FVM_SRECORD_U32;
}
/* Canonical decimal names have no sign, whitespace or leading zero aliases. */
static int fs_number(const char **cursor, unsigned long *value)
{
    const char *p = *cursor;
    unsigned long n = 0;
    if (*p < '1' || *p > '9')
        return 0;
    do
    {
        unsigned long digit = (unsigned long)(*p - '0');
        if (n > (FVM_SRECORD_U32 - digit) / 10UL)
            return 0;
        n = n * 10UL + digit;
        ++p;
    } while (*p >= '0' && *p <= '9');
    *cursor = p;
    *value = n;
    return 1;
}
/* Validate a descriptor without confusing it with a resource handle. */
static RFile *fs_file(FVM_RFS *fs, FVM_RFSFile h)
{
    RFile *file;
    if (!fs || !h.id || h.id > FVM_RFS_FILES || !fs_id(h.generation))
        return 0;
    file = fs->files + h.id - 1;
    return file->active && file->generation == h.generation ? file : 0;
}
/* Copy current authorized discovery; no handler or VM callback is called. */
static FVM_RResult fs_rows(FVM_RFS *fs, FVM_RResource *rows, size_t *count)
{
    return fvm_rdispatch_resources(fs->dispatch, fs->session, rows, FVM_R_RESOURCES, count);
}
/* Resolve an exact resource generation from a coherent authorized snapshot. */
static FVM_RResource *fs_row(FVM_RResource *rows, size_t count, FVM_RHandle h)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (rows[i].target.id == h.id && rows[i].target.generation == h.generation)
            return rows + i;
    return 0;
}
/* Only canonical native nodes participate in the synthetic namespace. */
static int fs_node(FVM_RFSNode node)
{
    if (node.kind > FVM_RFS_CALL)
        return 0;
    if (node.kind == FVM_RFS_ROOT)
        return !node.target.id && !node.target.generation && !node.operation;
    return node.target.id && node.target.id <= FVM_R_RESOURCES && fs_id(node.target.generation) &&
           (node.kind == FVM_RFS_CALL ? fs_id(node.operation) : !node.operation);
}
/* Build metadata from already authorized rows; allocate no descriptor or subscription. */
static FVM_RResult fs_entry(FVM_RFS *fs, FVM_RFSNode node, FVM_RResource *rows, size_t count,
                            FVM_RFSEntry *out)
{
    FVM_RFSEntry entry;
    FVM_RResource *row;
    size_t i, child = 0;
    if (!fs_node(node))
        return FVM_R_INVALID;
    memset(&entry, 0, sizeof(entry));
    entry.node = node;
    entry.mode = FVM_RFS_READ;
    entry.directory =
        node.kind == FVM_RFS_ROOT || node.kind == FVM_RFS_RESOURCE || node.kind == FVM_RFS_OPS;
    if (node.kind == FVM_RFS_ROOT)
    {
        strcpy(entry.name, "/");
        *out = entry;
        return FVM_R_OK;
    }
    row = fs_row(rows, count, node.target);
    if (!row)
        return FVM_R_UNKNOWN;
    switch (node.kind)
    {
    case FVM_RFS_RESOURCE:
        sprintf(entry.name, "%lu.%lu", node.target.id, node.target.generation);
        break;
    case FVM_RFS_PATH:
        strcpy(entry.name, "path");
        child = FVM_RFS_PATH - FVM_RFS_RESOURCE;
        break;
    case FVM_RFS_OPS:
        strcpy(entry.name, "ops");
        child = FVM_RFS_OPS - FVM_RFS_RESOURCE;
        break;
    case FVM_RFS_EVENTS:
        if (!(row->rights & FVM_R_SUBSCRIBE))
            return FVM_R_DENIED;
        strcpy(entry.name, "events");
        child = FVM_RFS_EVENTS - FVM_RFS_RESOURCE;
        break;
    case FVM_RFS_CALL:
        for (i = 0; i < row->count && row->operations[i].id != node.operation; ++i)
        {
        }
        if (i == row->count)
            return FVM_R_DENIED;
        {
            FVM_RResult result = fvm_rdispatch_operation_index(fs->dispatch, fs->session,
                                                               node.target, node.operation, &i);
            if (result != FVM_R_OK)
                return result;
        }
        child = FVM_RFS_CALL - FVM_RFS_RESOURCE + i;
        sprintf(entry.name, "%lu", node.operation);
        entry.mode |= FVM_RFS_WRITE;
        break;
    default:
        return FVM_R_INVALID;
    }
    entry.qid_low = node.target.id * FVM_RFS_QID_STRIDE + (unsigned long)child;
    entry.qid_high = node.target.generation;
    *out = entry;
    return FVM_R_OK;
}
/* Compare semantic fields, never uninitialized padding or filtered-array tails. */
static int fs_same(const FVM_RResource *a, const FVM_RResource *b)
{
    size_t i;
    if (a->target.id != b->target.id || a->target.generation != b->target.generation ||
        a->rights != b->rights || a->count != b->count || strcmp(a->path, b->path))
        return 0;
    for (i = 0; i < a->count; ++i)
        if (a->operations[i].id != b->operations[i].id ||
            a->operations[i].right != b->operations[i].right)
            return 0;
    return 1;
}
/* Directory snapshots remain coherent across partial reads or fail explicitly on discovery change.
 */
static FVM_RResult fs_snapshot(FVM_RFS *fs, RFile *file)
{
    FVM_RResource rows[FVM_R_RESOURCES];
    size_t count, i;
    FVM_RResult result = fs_rows(fs, rows, &count);
    if (result != FVM_R_OK)
        return result;
    if (count != file->count)
        return FVM_R_CONFLICT;
    for (i = 0; i < count; ++i)
        if (!fs_same(rows + i, file->rows + i))
            return FVM_R_CONFLICT;
    return FVM_R_OK;
}
/* Encode one stream-local declaration. The capacity is a compile-time bound, not client input. */
static FVM_RResult fs_declaration(RFile *file, const char *type)
{
    FVM_SBinding binding;
    unsigned char payload[FVM_RFS_BUFFER];
    size_t used;
    if (file->declaration)
    {
        file->start = file->size = 0;
        return FVM_R_OK;
    }
    memset(&binding, 0, sizeof(binding));
    binding.tag = FVM_RFS_TAG;
    binding.version = FVM_RWIRE_VERSION;
    binding.type_size = (unsigned int)strlen(type);
    strcpy(binding.type, type);
    if (fvm_sbindings_payload_write(&binding, 1, payload, sizeof(payload), &used) != FVM_S_OK ||
        fvm_srecord_write(0, payload, used, file->bytes, sizeof(file->bytes), &file->size) !=
            FVM_S_OK)
        return FVM_R_HANDLER;
    file->start = 0;
    file->declaration = 1;
    return FVM_R_OK;
}
/* Allocate only fixed native storage; no dispatcher mutation, rights or workers are implied. */
FVM_RResult fvm_rfs_create(FVM_RDispatch *dispatch, unsigned long session, FVM_RFS **out)
{
    FVM_RFS *fs;
    size_t i;
    if (!dispatch || !fs_id(session) || !out)
        return FVM_R_INVALID;
    fs = (FVM_RFS *)calloc(1, sizeof(*fs));
    if (!fs)
        return FVM_R_NOMEM;
    fs->dispatch = dispatch;
    fs->session = session;
    for (i = 0; i < FVM_RFS_FILES; ++i)
        fs->files[i].generation = 1;
    *out = fs;
    return FVM_R_OK;
}
/* Recheck current authority before returning copied metadata. */
FVM_RResult fvm_rfs_stat(FVM_RFS *fs, FVM_RFSNode node, FVM_RFSEntry *out)
{
    FVM_RResource rows[FVM_R_RESOURCES];
    size_t count;
    FVM_RResult result;
    if (!fs || !out)
        return FVM_R_INVALID;
    result = fs_rows(fs, rows, &count);
    return result == FVM_R_OK ? fs_entry(fs, node, rows, count, out) : result;
}
/* Resolve a component without following an OS path or substituting a new resource incarnation. */
FVM_RResult fvm_rfs_walk(FVM_RFS *fs, FVM_RFSNode parent, const char *name, FVM_RFSEntry *out)
{
    FVM_RFSEntry entry;
    FVM_RFSNode node;
    const char *p;
    FVM_RResult result;
    if (!fs || !name || !out)
        return FVM_R_INVALID;
    result = fvm_rfs_stat(fs, parent, &entry);
    if (result != FVM_R_OK)
        return result;
    if (!entry.directory)
        return FVM_R_INVALID;
    node = parent;
    if (!strcmp(name, ".."))
    {
        if (parent.kind == FVM_RFS_RESOURCE)
            memset(&node, 0, sizeof(node));
        else if (parent.kind == FVM_RFS_OPS)
            node.kind = FVM_RFS_RESOURCE;
    }
    else if (parent.kind == FVM_RFS_ROOT)
    {
        p = name;
        memset(&node, 0, sizeof(node));
        node.kind = FVM_RFS_RESOURCE;
        if (!fs_number(&p, &node.target.id) || *p++ != '.' ||
            !fs_number(&p, &node.target.generation) || *p)
            return FVM_R_UNKNOWN;
    }
    else if (parent.kind == FVM_RFS_RESOURCE)
    {
        if (!strcmp(name, "path"))
            node.kind = FVM_RFS_PATH;
        else if (!strcmp(name, "ops"))
            node.kind = FVM_RFS_OPS;
        else if (!strcmp(name, "events"))
            node.kind = FVM_RFS_EVENTS;
        else
            return FVM_R_UNKNOWN;
    }
    else
    {
        p = name;
        node.kind = FVM_RFS_CALL;
        if (!fs_number(&p, &node.operation) || *p)
            return FVM_R_UNKNOWN;
    }
    return fvm_rfs_stat(fs, node, out);
}
/* Publish the descriptor only after discovery and any required watch have succeeded. */
FVM_RResult fvm_rfs_open(FVM_RFS *fs, FVM_RFSNode node, unsigned int mode, FVM_RFSFile *out)
{
    FVM_RFSEntry entry;
    RFile *file;
    FVM_RFSFile h;
    size_t i, count;
    FVM_RResource rows[FVM_R_RESOURCES];
    FVM_RWatch watch;
    FVM_RResult result;
    if (!fs || !out)
        return FVM_R_INVALID;
    result = fvm_rfs_stat(fs, node, &entry);
    if (result != FVM_R_OK)
        return result;
    if (mode != entry.mode)
        return FVM_R_DENIED;
    for (i = 0; i < FVM_RFS_FILES && (fs->files[i].active || !fs->files[i].generation); ++i)
    {
    }
    if (i == FVM_RFS_FILES)
        return FVM_R_LIMIT;
    file = fs->files + i;
    memset(&watch, 0, sizeof(watch));
    count = 0;
    if (entry.directory)
    {
        result = fs_rows(fs, rows, &count);
        if (result != FVM_R_OK)
            return result;
    }
    if (node.kind == FVM_RFS_EVENTS)
    {
        result = fvm_rdispatch_subscribe(fs->dispatch, fs->session, node.target, &watch);
        if (result != FVM_R_OK)
            return result;
    }
    h.id = (unsigned long)i + 1;
    h.generation = file->generation;
    memset(file, 0, sizeof(*file));
    file->generation = h.generation;
    file->active = 1;
    file->node = node;
    file->watch = watch;
    file->count = count;
    if (count)
        memcpy(file->rows, rows, count * sizeof(*rows));
    *out = h;
    return FVM_R_OK;
}
/* Enumerate stable directory entries; no raw host Dir structs cross a transport. */
FVM_RResult fvm_rfs_readdir(FVM_RFS *fs, FVM_RFSFile h, size_t index, FVM_RFSEntry *out)
{
    RFile *file = fs_file(fs, h);
    FVM_RFSNode node;
    FVM_RResource *row;
    FVM_RResult result;
    if (!fs || !out)
        return FVM_R_INVALID;
    if (!file)
        return FVM_R_UNKNOWN;
    node = file->node;
    if (node.kind != FVM_RFS_ROOT && node.kind != FVM_RFS_RESOURCE && node.kind != FVM_RFS_OPS)
        return FVM_R_INVALID;
    result = fs_snapshot(fs, file);
    if (result != FVM_R_OK)
        return result;
    if (node.kind == FVM_RFS_ROOT)
    {
        if (index >= file->count)
            return FVM_R_PENDING;
        node.target = file->rows[index].target;
        node.kind = FVM_RFS_RESOURCE;
    }
    else
    {
        row = fs_row(file->rows, file->count, node.target);
        if (!row)
            return FVM_R_UNKNOWN;
        if (node.kind == FVM_RFS_RESOURCE)
        {
            if (index == 0)
                node.kind = FVM_RFS_PATH;
            else if (index == 1)
                node.kind = FVM_RFS_OPS;
            else if (index == 2 && (row->rights & FVM_R_SUBSCRIBE))
                node.kind = FVM_RFS_EVENTS;
            else
                return FVM_R_PENDING;
        }
        else
        {
            if (index >= row->count)
                return FVM_R_PENDING;
            node.kind = FVM_RFS_CALL;
            node.operation = row->operations[index].id;
        }
    }
    return fs_entry(fs, node, file->rows, file->count, out);
}
/* Prepare a complete bounded stream record before exposing any of its bytes. */
static FVM_RResult fs_prepare(FVM_RFS *fs, RFile *file)
{
    FVM_RReply reply;
    FVM_REvent event;
    FVM_RResult result;
    FVM_SResult status;
    size_t used;
    if (file->node.kind == FVM_RFS_CALL)
    {
        if (!file->written)
            return FVM_R_PENDING;
        if (!file->request)
            return FVM_R_OK;
        result = fvm_rdispatch_take(fs->dispatch, fs->session, file->request, &reply);
        if (result != FVM_R_OK)
            return result;
        result = fs_declaration(file, FVM_RWIRE_REPLY_TYPE);
        if (result != FVM_R_OK)
            return result;
        status =
            fvm_rreply_record_write(FVM_RFS_TAG, file->request, &reply, file->bytes + file->size,
                                    sizeof(file->bytes) - file->size, &used);
        file->request = 0;
    }
    else
    {
        /* Reserve the worst-case stream increment before consuming an event near Uint32 end. */
        if (file->offset > FVM_SRECORD_U32 - FVM_RFS_BUFFER)
            return FVM_R_LIMIT;
        result = fvm_rdispatch_event_take(fs->dispatch, fs->session, file->watch, &event);
        if (result != FVM_R_OK)
            return result;
        result = fs_declaration(file, FVM_RWIRE_EVENT_TYPE);
        if (result != FVM_R_OK)
            return result;
        status = fvm_revent_record_write(FVM_RFS_TAG, &event, file->bytes + file->size,
                                         sizeof(file->bytes) - file->size, &used);
    }
    if (status != FVM_S_OK)
        return FVM_R_HANDLER;
    file->size += used;
    return FVM_R_OK;
}
/* Partial output retains a private copy; no output is returned after a failed rights check. */
FVM_RResult fvm_rfs_read(FVM_RFS *fs, FVM_RFSFile h, unsigned long offset, void *output,
                         size_t capacity, size_t *used)
{
    RFile *file = fs_file(fs, h);
    FVM_RFSEntry entry;
    FVM_RResult result;
    size_t count;
    FVM_RResource rows[FVM_R_RESOURCES];
    size_t row_count;
    FVM_RResource *row;
    if (!fs || !output || !capacity || !used || offset > FVM_SRECORD_U32)
        return FVM_R_INVALID;
    if (!file)
        return FVM_R_UNKNOWN;
    result = fvm_rfs_stat(fs, file->node, &entry);
    if (result != FVM_R_OK)
        return result;
    if (file->node.kind == FVM_RFS_PATH)
    {
        result = fs_rows(fs, rows, &row_count);
        if (result != FVM_R_OK)
            return result;
        row = fs_row(rows, row_count, file->node.target);
        if (!row)
            return FVM_R_UNKNOWN;
        count = strlen(row->path);
        if (offset >= count)
            count = 0;
        else
        {
            count -= (size_t)offset;
            if (count > capacity)
                count = capacity;
            memcpy(output, row->path + offset, count);
        }
        *used = count;
        return FVM_R_OK;
    }
    if (entry.directory || offset != file->offset)
        return FVM_R_INVALID;
    if (file->node.kind == FVM_RFS_EVENTS)
    {
        result = fvm_rdispatch_watch_check(fs->dispatch, fs->session, file->watch);
        if (result != FVM_R_OK)
            return result;
    }
    if (file->start == file->size)
    {
        result = fs_prepare(fs, file);
        if (result != FVM_R_OK)
            return result;
    }
    count = file->size - file->start;
    if (count > capacity)
        count = capacity;
    if (count)
        memcpy(output, file->bytes + file->start, count);
    file->start += count;
    file->offset += (unsigned long)count;
    *used = count;
    return FVM_R_OK;
}
/* Queue one request with a server-assigned ID. The file never supplies a session on the wire. */
FVM_RResult fvm_rfs_write(FVM_RFS *fs, FVM_RFSFile h, unsigned long offset, const void *data,
                          size_t size, size_t *used)
{
    RFile *file = fs_file(fs, h);
    FVM_RFSEntry entry;
    FVM_RRequest request;
    FVM_RResult result;
    if (!fs || !used || (!data && size) || offset)
        return FVM_R_INVALID;
    if (!file)
        return FVM_R_UNKNOWN;
    if (file->node.kind != FVM_RFS_CALL)
        return FVM_R_DENIED;
    if (size > FVM_R_BYTES)
        return FVM_R_LIMIT;
    if (file->written)
        return FVM_R_CONFLICT;
    result = fvm_rfs_stat(fs, file->node, &entry);
    if (result != FVM_R_OK)
        return result;
    if (fs->next == FVM_SRECORD_U32)
        return FVM_R_LIMIT;
    memset(&request, 0, sizeof(request));
    request.session = fs->session;
    request.id = fs->next + 1;
    request.target = file->node.target;
    request.operation = file->node.operation;
    request.data = data;
    request.size = size;
    result = fvm_rdispatch_post(fs->dispatch, &request);
    if (result != FVM_R_OK)
        return result;
    fs->next = request.id;
    file->request = request.id;
    file->written = 1;
    *used = size;
    return FVM_R_OK;
}
/* Close cancels only work not yet started. A running native handler keeps the descriptor alive. */
FVM_RResult fvm_rfs_close(FVM_RFS *fs, FVM_RFSFile h)
{
    RFile *file = fs_file(fs, h);
    FVM_RResult result;
    FVM_RReply reply;
    unsigned long generation;
    if (!fs)
        return FVM_R_INVALID;
    if (!file)
        return FVM_R_UNKNOWN;
    if (file->request)
    {
        result = fvm_rdispatch_cancel(fs->dispatch, fs->session, file->request);
        if (result != FVM_R_OK && result != FVM_R_CONFLICT && result != FVM_R_UNKNOWN)
            return result;
        result = fvm_rdispatch_take(fs->dispatch, fs->session, file->request, &reply);
        if (result != FVM_R_OK && result != FVM_R_UNKNOWN)
            return result;
    }
    if (file->watch.id)
    {
        result = fvm_rdispatch_unsubscribe(fs->dispatch, fs->session, file->watch);
        if (result != FVM_R_OK && result != FVM_R_UNKNOWN)
            return result;
    }
    generation = file->generation;
    memset(file, 0, sizeof(*file));
    file->generation = generation == FVM_SRECORD_U32 ? 0 : generation + 1;
    return FVM_R_OK;
}
/* The host closes every descriptor and quiesces callers before releasing borrowed dispatcher use.
 */
FVM_RResult fvm_rfs_destroy(FVM_RFS **pointer)
{
    FVM_RFS *fs;
    size_t i;
    if (!pointer || !(fs = *pointer))
        return FVM_R_INVALID;
    for (i = 0; i < FVM_RFS_FILES; ++i)
        if (fs->files[i].active)
            return FVM_R_BUSY;
    free(fs);
    *pointer = 0;
    return FVM_R_OK;
}
