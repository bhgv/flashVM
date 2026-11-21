/* Copyright (c) 2026 bhgv
 * EN: Scoped selector leases for fixed arrays of live VFS integer cells.
 * RU: Выбор элемента фиксированного массива живых ячеек VFS.
 */
#include "cap_array_index.h"
#include "cancel.h"
#include "disvm/native_context.h"
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "channel_sync.h"
#include <stdlib.h>
#include <string.h>

typedef struct Selection
{
    struct Selection *next; /* Borrowed next selector while linked. */
    DVM_ProviderOpenScope scope; /* Open lifetime identity. */
    unsigned index; /* Selected cell, fixed after the complete write. */
    int selected; /* Selection established. */
    int alive; /* Atomic revocation checked by value fids. */
    size_t references; /* Selector and value fids, under the index lock. */
} Selection;

typedef struct ArrayIndex
{
    FVM_PortSync *lock; /* Serializes matching, selection and reference updates. */
    unsigned references; /* Endpoint factories and builder. */
    Selection *active; /* Open selectors. */
    unsigned count; /* Number of owned integer cell providers. */
    DVM_Provider *cells[]; /* Retained array of cell providers. */
} ArrayIndex;

typedef struct IndexNode
{
    ArrayIndex *array; /* Owned shared index. */
    int selector; /* One for .idx, zero for .val. */
} IndexNode;

typedef struct IndexCursor
{
    IndexNode *node; /* Borrowed from the pinned provider. */
    Selection *selection; /* Owned fid reference. */
    DVM_VfsHandle value; /* Selected cell cursor for .val only. */
} IndexCursor;


/* EN: Capture the reverse grant or the active local Dis process.
 * RU: Определить встречное разрешение или текущий локальный процесс Dis. */
DVM_ProviderOpenScope dvm_collection_open_scope(void)
{
    DVM_ProviderOpenScope scope = dvm_provider_current_open_scope();
    if (!scope.generation)
    {
        Prog *process = dvm_native_context_process(dvm_native_context_current());
        scope = (DVM_ProviderOpenScope){process, process ? UINT64_C(1) << 63 | (uint32_t)process->pid : 0};
    }
    return scope;
}


/* EN: Release the matching selector or value reference, revoking on selector close.
 * RU: Освободить ссылку выбора или значения и отозвать выбор при закрытии .idx. */
static void index_close(void *state)
{
    IndexCursor *cursor = state;
    dvm_vfs_close(&cursor->value);
    ArrayIndex *array = cursor->node->array;
    Selection *selection = cursor->selection;
    fvm_port_sync_lock(array->lock);
    if (cursor->node->selector)
    {
        Selection **link = &array->active;
        while (*link && *link != selection)
        {
            link = &(*link)->next;
        }
        if (*link)
        {
            *link = selection->next;
        }
        __atomic_store_n(&selection->alive, 0, __ATOMIC_RELEASE);
    }
    int last = --selection->references == 0;
    fvm_port_sync_unlock(array->lock);
    if (last)
    {
        free(selection);
    }
    free(cursor);
}


/* EN: Describe either exact-root endpoint.
 * RU: Описать один из двух файлов индексированного доступа. */
static FVM_Result index_info(void *state, const char *path, FVM_FileInfo *info)
{
    IndexNode *node = state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    *info = (FVM_FileInfo){0};
    strcpy(info->name, node->selector ? ".idx" : ".val");
    info->mode = node->selector ? 0222 : 0666;
    info->length.low = 4;
    return FVM_OK;
}


/* EN: Describe the open endpoint.
 * RU: Описать открытый файл выбора или значения. */
static FVM_Result index_stat(void *state, FVM_FileInfo *info)
{
    return index_info(((IndexCursor *)state)->node, ".", info);
}


/* EN: Read from the selected cell if the selector remains alive.
 * RU: Прочитать выбранную ячейку, пока открыт файл выбора. */
static void index_read(void *state, void *bytes, size_t size, DVM_IoResult *result)
{
    IndexCursor *cursor = state;
    if (!cursor->node->selector && __atomic_load_n(&cursor->selection->alive, __ATOMIC_ACQUIRE))
    {
        *result = dvm_vfs_read_chunk(&cursor->value, bytes, size);
        return;
    }
    *result = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
}


/* EN: Commit one complete selector word or forward a write to the live cell.
 * RU: Принять одно полное слово выбора или передать запись живой ячейке. */
static void index_write(void *state, const void *bytes, size_t size, DVM_IoResult *result)
{
    IndexCursor *cursor = state;
    Selection *selection = cursor->selection;
    *result = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        result->stop = DVM_IO_CANCELLED;
        return;
    }
    if (!cursor->node->selector)
    {
        if (__atomic_load_n(&selection->alive, __ATOMIC_ACQUIRE))
        {
            *result = dvm_vfs_write_chunk(&cursor->value, bytes, size);
        }
        return;
    }
    if (size != 4)
    {
        return;
    }
    const unsigned char *word = bytes;
    uint32_t index = (uint32_t)word[0] << 24 | (uint32_t)word[1] << 16 | (uint32_t)word[2] << 8 | word[3];
    ArrayIndex *array = cursor->node->array;
    fvm_port_sync_lock(array->lock);
    if (!selection->selected && index < array->count)
    {
        selection->index = index;
        selection->selected = 1;
        *result = (DVM_IoResult){.status = FVM_OK, .count = size, .flags = DVM_IO_PREFIX};
    }
    fvm_port_sync_unlock(array->lock);
}


/* EN: Seek on an active value fid using its independent cell cursor.
 * RU: Изменить позицию активного файла значения через его курсор. */
static FVM_Result index_seek(void *state, long offset, int origin, long *position)
{
    IndexCursor *cursor = state;
    if (cursor->node->selector)
    {
        if (!offset && origin == FVM_FILE_SEEK_START)
        {
            *position = 0;
            return FVM_OK;
        }
        return FVM_ERROR;
    }
    return __atomic_load_n(&cursor->selection->alive, __ATOMIC_ACQUIRE) ?
        dvm_vfs_seek(&cursor->value, offset, origin, position) : FVM_ERROR;
}


/* EN: Find the caller's selection while holding the array lock.
 * RU: Найти выбор клиента при удерживаемом замке массива. */
static Selection *find_selection(ArrayIndex *array, DVM_ProviderOpenScope scope)
{
    for (Selection *item = array->active; item; item = item->next)
    {
        if (item->scope.domain == scope.domain && item->scope.generation == scope.generation)
        {
            return item;
        }
    }
    return NULL;
}


/* EN: Open one selector per caller or a value for the caller's active selection.
 * RU: Открыть один выбор на клиента или значение активного выбора клиента. */
static FVM_Result index_open(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    static const DVM_VfsOps ops = {.info = index_stat, .seek = index_seek, .close = index_close,
        .read_chunk = index_read, .write_chunk = index_write};
    IndexNode *node = state;
    DVM_ProviderOpenScope scope = dvm_collection_open_scope();
    if (strcmp(path, ".") || !scope.domain || (node->selector && strcmp(mode, "wo")) ||
        dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    IndexCursor *cursor = calloc(1, sizeof(*cursor));
    Selection *created = node->selector ? calloc(1, sizeof(*created)) : NULL;
    if (!cursor || (node->selector && !created))
    {
        free(created);
        free(cursor);
        return FVM_OUT_OF_MEMORY;
    }
    ArrayIndex *array = node->array;
    DVM_Provider *cell = NULL;
    fvm_port_sync_lock(array->lock);
    Selection *selection = find_selection(array, scope);
    if (created && !selection)
    {
        *created = (Selection){.next = array->active, .scope = scope, .alive = 1, .references = 1};
        array->active = cursor->selection = created;
        created = NULL;
    }
    else if (!node->selector && selection && selection->selected && selection->references < SIZE_MAX)
    {
        selection->references++;
        cursor->selection = selection;
        cell = array->cells[selection->index];
    }
    fvm_port_sync_unlock(array->lock);
    free(created);
    if (!cursor->selection)
    {
        free(cursor);
        return FVM_ERROR;
    }
    cursor->node = node;
    if (cell && dvm_provider_open(cell, ".", mode, &cursor->value) != FVM_OK)
    {
        index_close(cursor);
        return FVM_ERROR;
    }
    out->state = cursor;
    out->ops = &ops;
    out->native_flags = DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_INFO | DVM_VFS_CANCEL_SEEK | DVM_VFS_CANCEL_CLOSE;
    return FVM_OK;
}


/* EN: Release one factory or builder reference and its retained cells on last drop.
 * RU: Освободить ссылку фабрики либо сборщика и ячейки при последней ссылке. */
static void release_array(ArrayIndex *array)
{
    if (__atomic_sub_fetch(&array->references, 1, __ATOMIC_ACQ_REL))
    {
        return;
    }
    for (unsigned i = 0; i < array->count; i++)
    {
        dvm_provider_release(array->cells[i]);
    }
    fvm_port_sync_destroy(array->lock);
    free(array);
}


/* EN: Consume one endpoint and release its array reference.
 * RU: Освободить точку доступа и её ссылку на массив. */
static void destroy_node(void *state)
{
    IndexNode *node = state;
    release_array(node->array);
    free(node);
}


/* EN: Map one exact-root endpoint into the caller's view.
 * RU: Отобразить конечный файл в представление вызывающего. */
static int map_node(DVM_View *view, ArrayIndex *array, int selector)
{
    static const DVM_ProviderOps ops = {.info = index_info, .open = index_open, .destroy = destroy_node,
        .flags = DVM_PROVIDER_STABLE_BIND};
    IndexNode *node = malloc(sizeof(*node));
    if (!node)
    {
        return 0;
    }
    *node = (IndexNode){array, selector};
    __atomic_add_fetch(&array->references, 1, __ATOMIC_RELAXED);
    DVM_Provider *provider = dvm_provider_create_native(&ops, node, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    if (!provider)
    {
        destroy_node(node);
        return 0;
    }
    unsigned access = selector ? FVM_ACCESS_WRITE : FVM_ACCESS_READ | FVM_ACCESS_WRITE;
    int mapped = dvm_view_map(view, selector ? "/.idx" : "/.val", provider, ".", access, 0);
    dvm_provider_release(provider);
    return mapped;
}


/* EN/RU: See cap_array_index.h. / См. cap_array_index.h. */
int dvm_intarray_map_index(DVM_View *view, DVM_Provider *const *cells, unsigned count)
{
    ArrayIndex *array = calloc(1, sizeof(*array) + count * sizeof(*cells));
    if (!array)
    {
        return 0;
    }
    array->lock = fvm_port_sync_create();
    if (!array->lock)
    {
        free(array);
        return 0;
    }
    array->references = 1;
    for (unsigned i = 0; i < count; i++)
    {
        if (!dvm_provider_retain(cells[i]))
        {
            release_array(array);
            return 0;
        }
        array->cells[array->count++] = cells[i];
    }
    int result = map_node(view, array, 1) && map_node(view, array, 0);
    release_array(array);
    return result;
}
