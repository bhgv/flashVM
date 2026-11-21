/* Copyright (c) 2026 bhgv
 * EN: Fixed Limbo arrays backed by live native integer cells and VFS metadata.
 * RU: Фиксированные массивы Limbo из живых нативных ячеек и метаданных VFS.
 */
#include "cap_internal.h"
#include "cancel.h"
#include "view_internal.h"
#include "cap_array_index.h"
#include <stdio.h>

#define DVM_INT_ARRAY_LIMIT (DVM_VIEW_RULE_LIMIT - 4U) /* Reserve two metadata and two selector nodes. */

typedef struct ArrayMetadata
{
    size_t length; /* Number of bytes before the terminating NUL. */
    char text[32]; /* Immutable UTF-8 text. */
} ArrayMetadata;

typedef struct MetadataReader
{
    const ArrayMetadata *file; /* Borrowed from the retained provider. */
    size_t offset; /* Independent open cursor. */
} MetadataReader;


/* EN: Describe an immutable exact-root metadata file.
 * RU: Описать неизменяемый файл метаданных. */
static FVM_Result meta_info(void *state, const char *path, FVM_FileInfo *info)
{
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    *info = (FVM_FileInfo){0};
    info->mode = 0444;
    info->length.low = ((ArrayMetadata *)state)->length;
    return FVM_OK;
}


/* EN: Copy a prefix at the current cursor, reacting to cancellation.
 * RU: Копировать префикс с текущей позиции с учётом отмены. */
static void meta_read(void *state, void *bytes, size_t size, DVM_IoResult *result)
{
    MetadataReader *reader = state;
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        *result = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_CANCELLED, .flags = DVM_IO_PREFIX};
        return;
    }
    size_t remaining = reader->file->length - reader->offset;
    size_t copied = size < remaining ? size : remaining;
    memcpy(bytes, reader->file->text + reader->offset, copied);
    reader->offset += copied;
    *result = (DVM_IoResult){.status = FVM_OK, .count = copied,
        .stop = copied ? DVM_IO_NONE : DVM_IO_EOF, .flags = DVM_IO_PREFIX};
}


/* EN: Describe a metadata cursor through the retained file.
 * RU: Описать открытый курсор через удерживаемый файл метаданных. */
static FVM_Result meta_stat(void *state, FVM_FileInfo *info)
{
    return meta_info((void *)((MetadataReader *)state)->file, ".", info);
}


/* EN: Set an independent cursor only within the immutable file.
 * RU: Установить независимый курсор в пределах неизменяемого файла. */
static FVM_Result meta_seek(void *state, long offset, int origin, long *result)
{
    MetadataReader *reader = state;
    long extent = (long)reader->file->length;
    long base = origin == FVM_FILE_SEEK_START ? 0 :
        origin == FVM_FILE_SEEK_CURRENT ? (long)reader->offset : origin == FVM_FILE_SEEK_END ? extent : -1;
    if (base < 0 || offset < -base || offset > extent - base || dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    *result = base + offset;
    reader->offset = (size_t)*result;
    return FVM_OK;
}


/* EN: Allocate a read-only cursor on the borrowed immutable metadata.
 * RU: Выделить курсор чтения неизменяемых заимствованных метаданных. */
static FVM_Result meta_open(void *state, const char *path, const char *mode, DVM_VfsHandle *handle)
{
    static const DVM_VfsOps ops = {.info = meta_stat, .read_chunk = meta_read, .seek = meta_seek, .close = free};
    if (strcmp(path, ".") || strcmp(mode, "r"))
    {
        return FVM_ERROR;
    }
    MetadataReader *reader = calloc(1, sizeof(*reader));
    if (!reader)
    {
        return FVM_OUT_OF_MEMORY;
    }
    reader->file = state;
    handle->state = reader;
    handle->ops = &ops;
    handle->native_flags = DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_INFO | DVM_VFS_CANCEL_SEEK | DVM_VFS_CANCEL_CLOSE;
    return FVM_OK;
}


/* EN: Copy text into a read-only node and transfer the view's reference.
 * RU: Копировать текст в файл только для чтения и передать ссылку представлению. */
static int map_meta(DVM_View *view, const char *path, const char *text)
{
    static const DVM_ProviderOps ops = {.info = meta_info, .open = meta_open, .destroy = free,
        .flags = DVM_PROVIDER_STABLE_BIND};
    ArrayMetadata *file = calloc(1, sizeof(*file));
    if (!file)
    {
        return 0;
    }
    file->length = strlen(text);
    memcpy(file->text, text, file->length + 1);
    DVM_Provider *provider = dvm_provider_create_native(&ops, file, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    if (!provider)
    {
        free(file);
        return 0;
    }
    int mapped = dvm_view_map(view, path, provider, ".", FVM_ACCESS_READ, 0);
    dvm_provider_release(provider);
    return mapped;
}


/* EN: Pin existing live cells at decimal names and publish their immutable extent.
 * RU: Удержать живые ячейки под десятичными именами и опубликовать неизменяемую длину. */
static DVM_Provider *array_view(List *cells)
{
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view)
    {
        return NULL;
    }
    DVM_Provider *providers[DVM_INT_ARRAY_LIMIT];
    unsigned count = 0;
    for (List *cell = cells; cell != H; cell = cell->tail)
    {
        void *value;
        memcpy(&value, cell->data, sizeof(value));
        DVM_CapState *state = dvm_cap_try_root(value);
        char path[16];
        snprintf(path, sizeof(path), "/%u", count);
        if (count >= DVM_INT_ARRAY_LIMIT || !dvm_cap_is_integer(state) ||
            !dvm_view_map(view, path, state->root, ".", FVM_ACCESS_READ | FVM_ACCESS_WRITE, 0))
        {
            dvm_view_discard(view);
            return NULL;
        }
        providers[count++] = state->root;
    }
    char length[16];
    snprintf(length, sizeof(length), "%u\n", count);
    if (!map_meta(view, "/.type", "array of int\n") || !map_meta(view, "/.len", length) ||
        !dvm_intarray_map_index(view, providers, count))
    {
        dvm_view_discard(view);
        return NULL;
    }
    return dvm_view_freeze(view);
}


/* EN: Create a fixed array capability from the caller's rooted list of integer cells.
 * RU: Создать фиксированный массив из укоренённого списка целочисленных ячеек. */
void Vfsexport_intarray(void *frame)
{
    F_Vfsexport_intarray call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_intarray, ret), sizeof(*call.ret));
    DVM_CapState *state;
    void *value = dvm_cap_new(DVM_CAP_ROOT, &state);
    if (state)
    {
        state->root = array_view(call.cells);
        if (!state->root)
        {
            dvm_cap_abandon(value);
            value = H;
        }
    }
    dvm_cap_publish(value, call.ret);
}
