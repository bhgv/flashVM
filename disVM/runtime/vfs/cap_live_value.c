/* Copyright (c) 2026 bhgv
 * EN: Publish live string and big cells with atomic replacement and per-read-fid snapshots.
 * RU: Опубликовать ячейки string и big с атомарной заменой и отдельным снимком на fid.
 */
#include "cap_internal.h"
#include "provider_internal.h"
#include "managed_value.h"
#include "cancel.h"
#include "../../native/terminal/live_variable.h"
#include "../../native/terminal/scalar_record.h"
#include "flashvm/utf8.h"


typedef struct LiveCell
{
    int big;                                   /* One signed big record when true. */
    FVM_PortSync *lock;                         /* Protects the wire value, never held across I/O. */
    DVM_Changes *journal;                       /* Borrowed from the owning .val provider. */
    size_t size;                                /* Complete framed extent. */
    unsigned char bytes[STYX_LIVE_STRING_FRAME]; /* Mutable snapshot in native memory. */
} LiveCell;

typedef struct LiveCursor
{
    LiveCell *cell;                           /* Retained by the opened provider. */
    size_t offset;                              /* Per-open byte position. */
    size_t size;                                /* Zero until the first successful read. */
    int readable;                               /* Granted read access. */
    int writable;                               /* Granted write access. */
    unsigned char bytes[STYX_LIVE_STRING_FRAME]; /* Immutable snapshot served by this cursor. */
} LiveCursor;


/* EN: Describe .val independently of the current string length.
 * RU: Описать .val независимо от текущей длины строки. */
static FVM_Result metadata(void *state, const char *path, FVM_FileInfo *info)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(info, 0, sizeof(*info));
    strcpy(info->name, ".val");
    info->mode = 0666;
    return FVM_OK;
}


/* EN: Serve a positional fragment of the first snapshot on this read fid.
 * RU: Выдать позиционный фрагмент первого снимка на этом fid чтения. */
static void read_value(void *state, void *bytes, size_t capacity, DVM_IoResult *out)
{
    LiveCursor *cursor = state;
    *out = (DVM_IoResult){.status = FVM_ERROR, .flags = DVM_IO_PREFIX, .stop = DVM_IO_DENIED};
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        out->stop = DVM_IO_CANCELLED;
    }
    else if (cursor->readable)
    {
        if (capacity && !cursor->size)
        {
            fvm_port_sync_lock(cursor->cell->lock);
            cursor->size = cursor->cell->size;
            memcpy(cursor->bytes, cursor->cell->bytes, cursor->size);
            fvm_port_sync_unlock(cursor->cell->lock);
        }
        size_t left = cursor->offset < cursor->size ? cursor->size - cursor->offset : 0;
        out->count = left < capacity ? left : capacity;
        if (out->count)
        {
            memcpy(bytes, cursor->bytes + cursor->offset, out->count);
            cursor->offset += out->count;
        }
        out->status = FVM_OK;
        out->stop = out->count ? DVM_IO_NONE : DVM_IO_EOF;
    }
}


/* EN: Validate one complete counted string or nil before taking the storage lock.
 * RU: Проверить одну полную строку с длиной или nil до захвата блокировки. */
static int valid_record(int big, const void *bytes, size_t count)
{
    FVM_SRecord value;
    size_t used;
    if (big)
    {
        StyxScalarKind kind;
        int64_t number;
        return styxscalarread(STYX_CALL_BIG_TAG, bytes, count, &kind, &number) && kind == STYX_SCALAR_VALUE;
    }
    return fvm_srecord_read(bytes, count, STYX_LIVE_STRING_FRAME, &value, &used) == FVM_S_OK &&
        used == count && value.size <= STYX_LIVE_STRING_LIMIT &&
        ((value.tag == STYX_MANAGED_NIL_TAG && !value.size) ||
            (value.tag == STYX_MANAGED_STRING_TAG &&
                fvm_utf8_validate((const char *)value.payload, value.size, NULL) == FVM_UTF8_OK));
}


/* EN: Replace the entire string on a confirmed offset-zero write.
 * RU: Заменить целую строку одной принятой записью со смещения ноль. */
static void write_value(void *state, const void *bytes, size_t count, DVM_IoResult *out)
{
    LiveCursor *cursor = state;
    *out = (DVM_IoResult){.status = FVM_ERROR, .flags = DVM_IO_PREFIX, .stop = DVM_IO_DENIED};
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        out->stop = DVM_IO_CANCELLED;
    }
    else if (cursor->writable && !cursor->offset && valid_record(cursor->cell->big, bytes, count))
    {
        LiveCell *cell = cursor->cell;
        fvm_port_sync_lock(cell->lock);
        int changed = cell->size != count || memcmp(cell->bytes, bytes, count);
        if (changed)
        {
            memcpy(cell->bytes, bytes, count);
            cell->size = count;
        }
        fvm_port_sync_unlock(cell->lock);
        if (changed)
        {
            changespost(cell->journal, DVM_CHANGE_CONTENT, 0, 0);
        }
        cursor->offset = count;
        out->status = FVM_OK;
        out->count = count;
        out->stop = DVM_IO_NONE;
    }
}


/* EN: Move the cursor in the bounded framed range, retaining any read snapshot.
 * RU: Переместить позицию в пределах кадра, сохраняя уже взятый снимок. */
static FVM_Result seek_value(void *state, long offset, int origin, long *position)
{
    LiveCursor *cursor = state;
    long base = origin == FVM_FILE_SEEK_START ? 0 :
        origin == FVM_FILE_SEEK_CURRENT ? (long)cursor->offset : -1;
    if (base < 0 || offset < -base || offset > (long)STYX_LIVE_STRING_FRAME - base)
    {
        return FVM_ERROR;
    }
    *position = base + offset;
    cursor->offset = (size_t)*position;
    return FVM_OK;
}

static const DVM_VfsOps stream = {.read_chunk = read_value, .write_chunk = write_value,
    .seek = seek_value, .close = free}; /* Each data handle owns its bounded snapshot. */


/* EN: Open a new independent cursor for a granted mode.
 * RU: Открыть независимую позицию с выданными правами. */
static FVM_Result open_value(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    int reading = !strcmp(mode, "r") || !strcmp(mode, "rw") || !strcmp(mode, "rwo");
    int writing = !strcmp(mode, "wo") || !strcmp(mode, "rw") || !strcmp(mode, "rwo");
    if ((!reading && !writing) || dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    LiveCursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    cursor->cell = state;
    cursor->readable = reading;
    cursor->writable = writing;
    out->state = cursor;
    out->ops = &stream;
    out->native_flags = DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_SEEK | DVM_VFS_CANCEL_CLOSE;
    return FVM_OK;
}


/* EN: Release the native storage and lock after the last node or open fid.
 * RU: Освободить нативное хранилище и блокировку после последнего узла или fid. */
static void destroy_value(void *state)
{
    LiveCell *cell = state;
    fvm_port_sync_destroy(cell->lock);
    free(cell);
}

static const DVM_ProviderOps factory = {.info = metadata, .open = open_value,
    .destroy = destroy_value, .flags = DVM_PROVIDER_STABLE_BIND}; /* Exact live scalar data node. */


/* EN: Copy one validated wire scalar into an independent live projection.
 * RU: Скопировать проверенный скаляр в независимую живую проекцию. */
static DVM_Provider *value_view(int big, const void *bytes, size_t size)
{
    if (!valid_record(big, bytes, size))
    {
        return NULL;
    }
    LiveCell *cell = calloc(1, sizeof(*cell));
    if (!cell || !(cell->lock = fvm_port_sync_create()))
    {
        free(cell);
        return NULL;
    }
    cell->big = big;
    cell->size = size;
    memcpy(cell->bytes, bytes, size);
    DVM_Provider *value = dvm_provider_create_native(&factory, cell, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    if (!value)
    {
        destroy_value(cell);
        return NULL;
    }
    cell->journal = dvm_provider_changes(value);
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    int mapped = view && dvm_cap_map_metadata(view, "/.type", big ? STYX_LIVE_BIG : STYX_LIVE_STRING) &&
        dvm_view_map(view, "/.val", value, ".", FVM_ACCESS_READ | FVM_ACCESS_WRITE, 0);
    dvm_provider_release(value);
    if (!mapped)
    {
        dvm_view_discard(view);
        return NULL;
    }
    return dvm_view_freeze(view);
}


/* [GC:] EN: Publish one exact native capability from an admitted initial record.
 * RU: Опубликовать точную нативную возможность из допущенной исходной записи. */
static void publish(int big, const void *bytes, size_t size, void *destination)
{
    DVM_CapState *state;
    void *value = dvm_cap_new(DVM_CAP_ROOT, &state);
    if (state)
    {
        state->root = value_view(big, bytes, size);
        if (!state->root)
        {
            dvm_cap_abandon(value);
            value = H;
        }
    }
    dvm_cap_publish(value, destination);
}


/* [GC:] EN: Copy the rooted initial string or nil from the captured frame.
 * RU: Скопировать удерживаемую исходную строку или nil из захваченного кадра. */
void Vfsexport_stringvar(void *frame)
{
    F_Vfsexport_stringvar call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_stringvar, ret), sizeof(*call.ret));
    unsigned char bytes[STYX_LIVE_STRING_FRAME];
    size_t size;
    if (dvm_styx_managed_write("s", dvm_sys_owner(&call.initial), bytes, sizeof(bytes), &size))
    {
        publish(0, bytes, size, call.ret);
    }
    else
    {
        dvm_cap_publish(H, call.ret);
    }
}


/* [GC:] EN: Publish a signed 64-bit scalar without allocating a managed string.
 * RU: Опубликовать знаковый 64-битный скаляр без выделения строки. */
void Vfsexport_bigvar(void *frame)
{
    F_Vfsexport_bigvar call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_bigvar, ret), sizeof(*call.ret));
    unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY];
    size_t size;
    if (styxscalarwrite(STYX_CALL_BIG_TAG, STYX_SCALAR_VALUE, call.initial, bytes, sizeof(bytes), &size))
    {
        publish(1, bytes, size, call.ret);
    }
    else
    {
        dvm_cap_publish(H, call.ret);
    }
}
