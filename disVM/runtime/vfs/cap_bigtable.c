/* Copyright (c) 2026 bhgv
 * EN: VFS command files backed by the same guarded storage as a Limbo Bigtable.
 * RU: Файлы операций VFS над тем же защищённым хранилищем, что и Limbo Bigtable.
 */
#include "cap_internal.h"
#include "bigtable_owner.h"
#include "table_request.h"
#include "cancel.h"
#include <errno.h>
#include <stdio.h>

#define TABLE_KEY_LIMIT 255U /* Maximum counted text key on one command write. */

typedef enum TableNodeKind
{
    Length, Count, Insert, Push, Pop, Add
} TableNodeKind;

typedef struct TableProjection
{
    struct TableStorage *storage; /* Owned native table reference. */
    unsigned references; /* One builder reference plus one per provider. */
} TableProjection;

typedef struct TableNode
{
    TableProjection *table; /* Owned projection. */
    TableNodeKind kind; /* Immutable file operation. */
    const char *name; /* Static basename. */
    int readable; /* Exact read versus write access. */
} TableNode;

typedef struct TableCursor
{
    TableNode *node; /* Borrowed from the pinned provider. */
    unsigned char data[32]; /* Metadata snapshot or cached pop result. */
    size_t size; /* Cached extent. */
    size_t offset; /* Individual cursor. */
    int completed; /* One command effect per open. */
} TableCursor;


/* EN: Decode one signed word without relying on signed conversion overflow.
 * RU: Декодировать знаковое слово без переполнения знакового преобразования. */
static int64_t word(const unsigned char *bytes)
{
    uint64_t bits = 0;
    for (unsigned i = 0; i < 8; i++)
    {
        bits = (bits << 8) | bytes[i];
    }
    return bits <= INT64_MAX ? (int64_t)bits : -1 - (int64_t)(UINT64_MAX - bits);
}


/* EN: Describe the exact command file.
 * RU: Описать точный файл операции. */
static FVM_Result table_info(void *state, const char *path, FVM_FileInfo *out)
{
    TableNode *node = state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    *out = (FVM_FileInfo){0};
    strcpy(out->name, node->name);
    out->mode = node->readable ? 0444 : 0222;
    out->length.low = node->kind == Pop ? 8 : 0;
    return FVM_OK;
}


/* EN: Describe the opened command file.
 * RU: Описать открытый файл операции. */
static FVM_Result table_stat(void *state, FVM_FileInfo *out)
{
    return table_info(((TableCursor *)state)->node, ".", out);
}


/* EN: Initialize a precise status and execute only an admitted native request.
 * RU: Установить точный статус и выполнить только допущенный нативный запрос. */
static int apply_request(TableCursor *cursor, FvmTableRequest *request, DVM_IoResult *out)
{
    *out = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        out->stop = DVM_IO_CANCELLED;
        return 0;
    }
    int error = dvm_bigtable_storage_request(cursor->node->table->storage, request);
    if (error == EBUSY)
    {
        out->stop = DVM_IO_AGAIN;
    }
    return !error && request->status == FHT_OK;
}


/* EN: Snapshot metadata or pop once, and handle short reads without replay.
 * RU: Снять метаданные или извлечь элемент один раз и обработать короткое чтение. */
static void table_read(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    TableCursor *cursor = state;
    *out = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
    if (!cursor->node->readable)
    {
        return;
    }
    if (!cursor->completed && size)
    {
        TableNodeKind kind = cursor->node->kind;
        FvmTableRequest request = {.operation = kind == Pop ? FVM_TABLE_POP :
            kind == Length ? FVM_TABLE_LENGTH : FVM_TABLE_COUNT};
        if (!apply_request(cursor, &request, out))
        {
            return;
        }
        if (kind == Pop)
        {
            uint64_t bits = (uint64_t)request.value;
            for (unsigned i = 0; i < 8; i++)
            {
                cursor->data[i] = (unsigned char)(bits >> (56 - i*8));
            }
            cursor->size = 8;
        }
        else
        {
            cursor->size = (size_t)snprintf((char *)cursor->data, sizeof(cursor->data),
                "%llu\n", (unsigned long long)request.value);
        }
        cursor->completed = 1;
    }
    size_t remaining = cursor->size - cursor->offset;
    size_t copied = size < remaining ? size : remaining;
    if (copied)
    {
        memcpy(bytes, cursor->data + cursor->offset, copied);
        cursor->offset += copied;
    }
    *out = (DVM_IoResult){.status = FVM_OK, .count = copied,
        .stop = copied ? DVM_IO_NONE : DVM_IO_EOF, .flags = DVM_IO_PREFIX};
}


/* EN: Decode exactly one operation and consume its fid on success.
 * RU: Декодировать ровно одну операцию и поглотить fid при успехе. */
static void table_write(void *state, const void *data, size_t size, DVM_IoResult *out)
{
    TableCursor *cursor = state;
    const unsigned char *bytes = data;
    *out = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
    if (cursor->completed || cursor->node->readable)
    {
        return;
    }
    FvmTableRequest request = {0};
    switch (cursor->node->kind)
    {
    case Push:
        if (size != 8)
        {
            return;
        }
        request.operation = FVM_TABLE_PUSH;
        break;
    case Insert:
        if (size != 16)
        {
            return;
        }
        request.operation = FVM_TABLE_INSERT;
        request.first = word(bytes);
        break;
    case Add:
        if (size < 9 || size > TABLE_KEY_LIMIT + 9)
        {
            return;
        }
        request.operation = FVM_TABLE_ADD;
        if (bytes[0] == 0 && size == 17)
        {
            request.key = (FhtKey){.kind = FHT_INDEX, .index = word(bytes + 1)};
        }
        else if (bytes[0] == 1)
        {
            request.key = (FhtKey){.kind = FHT_TEXT, .text = (const char *)bytes + 1, .size = size - 9};
        }
        else
        {
            return;
        }
        break;
    default:
        return;
    }
    request.value = word(bytes + size - 8);
    if (apply_request(cursor, &request, out))
    {
        cursor->completed = 1;
        *out = (DVM_IoResult){.status = FVM_OK, .count = size, .flags = DVM_IO_PREFIX};
    }
}


/* EN: Reposition reads in the same cached record without repeating a mutation.
 * RU: Переместить чтение по тому же снимку без повторения изменения. */
static FVM_Result table_seek(void *state, long offset, int origin, long *position)
{
    TableCursor *cursor = state;
    long length = (long)cursor->size;
    long base = origin == FVM_FILE_SEEK_START ? 0 :
        origin == FVM_FILE_SEEK_CURRENT ? (long)cursor->offset : origin == FVM_FILE_SEEK_END ? length : -1;
    if (base < 0 || offset < -base || offset > length - base)
    {
        return FVM_ERROR;
    }
    *position = base + offset;
    cursor->offset = (size_t)*position;
    return FVM_OK;
}


/* EN: Open a mode-checked cursor on one command file.
 * RU: Открыть курсор файла операции с проверкой режима. */
static FVM_Result table_open(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    static const DVM_VfsOps ops = {.info = table_stat, .seek = table_seek, .close = free,
        .read_chunk = table_read, .write_chunk = table_write};
    TableNode *node = state;
    if (strcmp(path, ".") || strcmp(mode, node->readable ? "r" : "wo"))
    {
        return FVM_ERROR;
    }
    TableCursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    cursor->node = node;
    out->state = cursor;
    out->ops = &ops;
    out->native_flags = DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_INFO | DVM_VFS_CANCEL_SEEK | DVM_VFS_CANCEL_CLOSE;
    return FVM_OK;
}


/* EN: Release the last native projection and drop the table storage pin.
 * RU: Освободить последнее представление и ссылку на хранилище. */
static void release_projection(TableProjection *table)
{
    if (__atomic_sub_fetch(&table->references, 1, __ATOMIC_ACQ_REL) == 0)
    {
        dvm_bigtable_storage_release(table->storage);
        free(table);
    }
}


/* EN: Consume a file factory and its projection reference.
 * RU: Освободить фабрику файла и её ссылку на представление. */
static void destroy_node(void *state)
{
    TableNode *node = state;
    release_projection(node->table);
    free(node);
}


/* EN: Map one local operation file into the virtual table directory.
 * RU: Отобразить файл операции в виртуальном каталоге таблицы. */
static int map_node(DVM_View *view, TableProjection *table, TableNodeKind kind, const char *path, int readable)
{
    static const DVM_ProviderOps ops = {.info = table_info, .open = table_open,
        .destroy = destroy_node, .flags = DVM_PROVIDER_STABLE_BIND};
    TableNode *node = malloc(sizeof(*node));
    if (!node)
    {
        return 0;
    }
    *node = (TableNode){table, kind, path + 1, readable};
    __atomic_add_fetch(&table->references, 1, __ATOMIC_RELAXED);
    DVM_Provider *provider = dvm_provider_create_native(&ops, node, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    if (!provider)
    {
        destroy_node(node);
        return 0;
    }
    int mapped = dvm_view_map(view, path, provider, ".", readable ? FVM_ACCESS_READ : FVM_ACCESS_WRITE, 0);
    dvm_provider_release(provider);
    return mapped;
}


/* EN: Pin an exact source table and publish a live VFS directory for it.
 * RU: Удержать точную таблицу и опубликовать её живой каталог VFS. */
static DVM_Provider *table_view(void *source)
{
    TableProjection *table = calloc(1, sizeof(*table));
    if (!table)
    {
        return NULL;
    }
    table->storage = dvm_bigtable_storage_retain(source);
    table->references = 1;
    DVM_View *view = table->storage ? dvm_view_create(NULL, NULL, NULL) : NULL;
    int mapped = view && dvm_cap_map_metadata(view, "/.type", "table of big\n") &&
        map_node(view, table, Length, "/.len", 1) && map_node(view, table, Count, "/.count", 1) &&
        map_node(view, table, Insert, "/.ins", 0) && map_node(view, table, Push, "/.push", 0) &&
        map_node(view, table, Pop, "/.pop", 1) && map_node(view, table, Add, "/.add", 0);
    release_projection(table);
    if (!mapped)
    {
        dvm_view_discard(view);
        return NULL;
    }
    return dvm_view_freeze(view);
}


/* EN: Build a capability for live table storage, retaining no managed reference.
 * RU: Создать возможность живой таблицы без удержания управляемой ссылки. */
void Vfsexport_table(void *frame)
{
    F_Vfsexport_table call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_table, ret), sizeof(*call.ret));
    DVM_CapState *state;
    void *value = dvm_cap_new(DVM_CAP_ROOT, &state);
    if (state)
    {
        state->root = table_view(call.source);
        if (!state->root)
        {
            dvm_cap_abandon(value);
            value = H;
        }
    }
    dvm_cap_publish(value, call.ret);
}
