/* Copyright (c) 2026 bhgv
 * EN: Pin and check a live VFS projection, performing one indexed transaction per operation.
 * RU: Удерживать и проверять проекцию VFS, выполняя одну индексную операцию за вызов.
 */
#include "styxrpc/live_client.h"
#include "styxrpc/managed.h"
#include "styxrpc/variable_file.h"
#include "vfs_client_internal.h"
#include "flashvm/utf8.h"
#include <stdlib.h>
#include <string.h>

#define STYX_LIVE_BIG_SIZE 8U /* Number of bytes in the native signed big wire word. */


struct StyxRpcLive
{
    StyxClient *session;                 /* Borrowed through the held exact fid. */
    StyxFid *fid;                        /* Owned unopened original node. */
    uint64_t identity;                   /* Qid.path exposed by the mounted peer. */
    StyxRpcLiveKind kind;                /* Checked immutable projection type. */
    int keyed;                           /* A table adds a numeric-key discriminator. */
    int writable;                       /* Upper bound on permitted writes. */
    char path[STYXRPC_VFS_PATH_MAX + 1]; /* Canonical relative path for revalidation. */
};

typedef struct LiveAccess
{
    StyxRpcVfsWait wait; /* Stop hook borrowed by all stages of one operation. */
    StyxFid *selector;   /* Owned until the mapped value file is closed. */
    StyxFid *value;      /* Owned opened data or metadata file. */
} LiveAccess;


/* EN: Validate variable paths, admitting .prog only before one exact .var subtree.
 * RU: Проверить путь переменной, допуская .prog только перед точным деревом .var. */
static int live_path(const char *path)
{
    if (!path || !*path || strlen(path) > STYXRPC_VFS_PATH_MAX)
    {
        return 0;
    }
    int program = 0, variable = 0;
    for (const char *start = path; *start;)
    {
        const char *end = strchr(start, '/');
        size_t length = end ? (size_t)(end - start) : strlen(start);
        if (!length || (end && !end[1]))
        {
            return 0;
        }
        if (start[0] == '.')
        {
            if (end && !program && !variable && length == 5 && !memcmp(start, ".prog", length))
            {
                program = 1;
            }
            else if (!variable && length == 4 && !memcmp(start, ".var", length))
            {
                variable = 1;
            }
            else
            {
                return 0;
            }
        }
        if (!end)
        {
            break;
        }
        start = end + 1;
    }
    return !program || variable;
}


/* EN: Open one named regular member relative to an unopened borrowed node.
 * RU: Открыть именованный обычный файл относительно заимствованного узла. */
static StyxRpcIoOutcome open_at(StyxFid *base, const char *name, StyxRpcFileMode mode,
    StyxRpcVfsWait *wait, StyxFid **out)
{
    StyxRpcIoOutcome status = styxrpc_vfs_control_walk_from(base, name, wait, out);
    if (status == STYXRPC_IO_OK)
    {
        StyxMeta meta;
        status = styxrpc_vfs_control_stat(*out, &meta, wait, STYXRPC_IO_NOT_OPEN);
        if (status == STYXRPC_IO_OK)
        {
            status = meta.directory ? STYXRPC_IO_NOT_OPEN : styxrpc_vfs_control_open(*out, mode, wait);
        }
    }
    return status;
}


/* EN: Read a complete bounded positional file while wait owns the per-operation stop flag.
 * RU: Прочитать полный ограниченный файл с флагом остановки операции в wait. */
static StyxRpcIoOutcome read_file(StyxFid *fid, void *bytes, size_t capacity, size_t *size, StyxRpcVfsWait *wait)
{
    return styxrpc_vfs_file_read_until(fid, 0, bytes, capacity, size,
        wait->stop ? styxrpc_vfs_control_poll : NULL, wait);
}


/* EN: Transfer one complete write at offset zero. Respect uncertainty and never send a suffix.
 * RU: Передать одну полную запись с нуля. Сохранять неопределённость и избегать досылки. */
static StyxRpcIoOutcome write_file(StyxFid *fid, const void *bytes, size_t size, StyxRpcVfsWait *wait)
{
    StyxTransfer part;
    if (styxrpc_vfs_control_poll(wait))
    {
        return STYXRPC_IO_CANCELLED;
    }
    styxclientwriteuntil(fid, 0, bytes, size, wait->stop ? styxrpc_vfs_control_poll : NULL, wait, &part);
    StyxRpcIoOutcome status = styxrpc_vfs_control_end(wait, NULL, &part, 1, STYXRPC_IO_FAILED);
    return status == STYXRPC_IO_OK && part.count != size ? STYXRPC_IO_FAILED : status;
}


/* EN: Match an exact type descriptor on the captured directory.
 * RU: Распознать полный описатель типа в удерживаемом каталоге. */
static StyxRpcIoOutcome directory_kind(StyxFid *fid, StyxRpcVfsWait *wait, StyxRpcLiveKind *kind, int *keyed)
{
    static const struct
    {
        const char *text;      /* Exact bytes, including newline. */
        StyxRpcLiveKind kind;  /* Public operation family. */
    } types[] = {
        {STYX_LIVE_INT_ARRAY, STYXRPC_LIVE_INTS}, {STYX_LIVE_BIG_VECTOR, STYXRPC_LIVE_BIGS},
        {STYX_LIVE_BIG_TABLE, STYXRPC_LIVE_BIGS}, {STYX_LIVE_RECORD, STYXRPC_LIVE_ROW},
        {STYX_LIVE_STRING, STYXRPC_LIVE_TEXT}, {STYX_LIVE_BIG, STYXRPC_LIVE_BIG}
    }; /* Known projection types. */
    char text[32];
    size_t used;
    StyxFid *file = NULL;
    StyxRpcIoOutcome status = open_at(fid, ".type", STYXRPC_FILE_READ, wait, &file);
    if (status == STYXRPC_IO_OK)
    {
        status = read_file(file, text, sizeof(text), &used, wait);
    }
    styxrpc_vfs_control_close(file, wait);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    for (size_t i = 0; i < sizeof(types) / sizeof(*types); ++i)
    {
        if (used == strlen(types[i].text) && !memcmp(text, types[i].text, used))
        {
            *kind = types[i].kind;
            *keyed = !strcmp(types[i].text, STYX_LIVE_BIG_TABLE);
            return STYXRPC_IO_OK;
        }
    }
    return STYXRPC_IO_NOT_OPEN;
}


/* EN: Capture the requested node and publish its type only after complete admission.
 * RU: Захватить запрошенный узел и выдать его тип после полной проверки. */
static StyxRpcIoOutcome capture(StyxClient *session, const char *path, int writable, StyxFid *base,
    const char *name, StyxRpcLive **out, StyxRpcVfsWait *wait)
{
    StyxFid *fid = NULL;
    StyxMeta meta;
    StyxRpcLiveKind kind = STYXRPC_LIVE_INT;
    int keyed = 0;
    StyxRpcIoOutcome status = base ? styxrpc_vfs_control_walk_from(base, name, wait, &fid) :
        styxrpc_vfs_control_walk(session, path, wait, &fid);
    if (status == STYXRPC_IO_OK)
    {
        status = styxrpc_vfs_control_stat(fid, &meta, wait, STYXRPC_IO_NOT_OPEN);
    }
    if (status == STYXRPC_IO_OK)
    {
        status = meta.directory ? directory_kind(fid, wait, &kind, &keyed) :
            (base || !strncmp(path, ".var/", 5) || strstr(path, "/.var/")) && meta.length == STYX_VARIABLE_INT_SIZE && (meta.mode & 0444U) &&
                (!writable || base || (meta.mode & 0222U)) ? STYXRPC_IO_OK : STYXRPC_IO_NOT_OPEN;
    }
    if (status == STYXRPC_IO_OK)
    {
        StyxRpcLive *live = malloc(sizeof(*live));
        if (live)
        {
            live->session = session;
            live->fid = fid;
            live->identity = meta.identity;
            live->kind = kind;
            live->keyed = keyed;
            live->writable = writable && (meta.directory || (meta.mode & 0222U));
            strcpy(live->path, path);
            *out = live;
            return STYXRPC_IO_OK;
        }
        status = STYXRPC_IO_FAILED;
    }
    styxrpc_vfs_control_close(fid, wait);
    return status;
}


/* EN: Revalidate the published identity. Subsequent access stays relative to the original fid.
 * RU: Перепроверить идентичность публикации. Доступ остаётся относительно исходного fid. */
static StyxRpcIoOutcome current(const StyxRpcLive *live, StyxRpcVfsWait *wait)
{
    StyxFid *visible = NULL;
    StyxMeta meta;
    if (!live)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcIoOutcome status = styxrpc_vfs_control_walk(live->session, live->path, wait, &visible);
    if (status == STYXRPC_IO_OK)
    {
        status = styxrpc_vfs_control_stat(visible, &meta, wait, STYXRPC_IO_NOT_OPEN);
        if (status == STYXRPC_IO_OK && (meta.identity != live->identity ||
            !!meta.directory != (live->kind != STYXRPC_LIVE_INT)))
        {
            status = STYXRPC_IO_NOT_OPEN;
        }
    }
    styxrpc_vfs_control_close(visible, wait);
    return status;
}


/* EN/RU: See live_client.h. / См. live_client.h. */
StyxRpcIoOutcome styxrpc_live_open_until(StyxClient *session, const char *path, int writable,
    StyxRpcLive **out, StyxRpcVfsStop stop, void *state)
{
    StyxRpcVfsWait wait = {stop, state, 0};
    return session && out && (writable == 0 || writable == 1) && live_path(path) ?
        capture(session, path, writable, NULL, NULL, out, &wait) : STYXRPC_IO_NOT_OPEN;
}


/* EN/RU: See live_client.h. / См. live_client.h. */
StyxRpcLiveKind styxrpc_live_kind(const StyxRpcLive *live)
{
    return live->kind;
}


/* EN/RU: See live_client.h. / См. live_client.h. */
StyxRpcIoOutcome styxrpc_live_field_until(const StyxRpcLive *live, const char *name, StyxRpcLive **out,
    StyxRpcVfsStop stop, void *state)
{
    StyxRpcVfsWait wait = {stop, state, 0};
    if (!live || live->kind != STYXRPC_LIVE_ROW || !out || !name || !*name || *name == '.' || strchr(name, '/'))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    size_t length = strlen(live->path);
    if (strlen(name) >= STYXRPC_VFS_PATH_MAX - length)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    char path[STYXRPC_VFS_PATH_MAX + 1];
    memcpy(path, live->path, length);
    path[length] = '/';
    strcpy(path + length + 1, name);
    StyxRpcIoOutcome status = current(live, &wait);
    return status == STYXRPC_IO_OK ?
        capture(live->session, path, live->writable, live->fid, name, out, &wait) : status;
}


/* EN/RU: See live_client.h. / См. live_client.h. */
StyxRpcIoOutcome styxrpc_live_length_until(const StyxRpcLive *live, uint32_t *length, StyxRpcVfsStop stop, void *state)
{
    LiveAccess access = {{stop, state, 0}, NULL, NULL};
    if (!live || !length || (live->kind != STYXRPC_LIVE_INTS && live->kind != STYXRPC_LIVE_BIGS &&
        live->kind != STYXRPC_LIVE_ROW))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcIoOutcome status = current(live, &access.wait);
    if (status == STYXRPC_IO_OK)
    {
        status = open_at(live->fid, ".len", STYXRPC_FILE_READ, &access.wait, &access.value);
    }
    char bytes[16];
    size_t count;
    if (status == STYXRPC_IO_OK)
    {
        status = read_file(access.value, bytes, sizeof(bytes), &count, &access.wait);
    }
    styxrpc_vfs_control_close(access.value, &access.wait);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    uint64_t decoded = 0;
    if (count < 2 || bytes[count - 1] != '\n')
    {
        return STYXRPC_IO_FAILED;
    }
    for (size_t i = 0; i + 1 < count; ++i)
    {
        if (bytes[i] < '0' || bytes[i] > '9' || (decoded = decoded * 10 + (unsigned)(bytes[i] - '0')) > UINT32_MAX)
        {
            return STYXRPC_IO_FAILED;
        }
    }
    *length = (uint32_t)decoded;
    return STYXRPC_IO_OK;
}


/* EN: Encode the low width network-order bytes of an exact signed value or index.
 * RU: Записать младшие width байт точного значения или индекса в сетевом порядке. */
static void pack_number(uint64_t value, unsigned char *bytes, size_t width)
{
    for (size_t i = width; i; value >>= 8)
    {
        bytes[--i] = (unsigned char)value;
    }
}


/* EN: Read a signed network-order word without implementation-defined unsigned casts.
 * RU: Прочитать знаковое слово сетевого порядка без зависящего от платформы преобразования. */
static int64_t unpack_number(const unsigned char *bytes, size_t width)
{
    int64_t value = bytes[0] < 128 ? 0 : -1;
    for (size_t i = 0; i < width; ++i)
    {
        value = value * 256 + bytes[i];
    }
    return value;
}


/* EN: Verify the exact publication, select one checked index when applicable, then open its value.
 * RU: Проверить точную публикацию, выбрать допустимый индекс и открыть файл значения. */
static StyxRpcIoOutcome open_value(const StyxRpcLive *live, int64_t index, int writing, LiveAccess *access)
{
    if (!live || live->kind == STYXRPC_LIVE_ROW || (writing && !live->writable) ||
        (live->kind == STYXRPC_LIVE_INTS && (index < 0 || index > UINT32_MAX)) ||
        (index && live->kind != STYXRPC_LIVE_INTS && live->kind != STYXRPC_LIVE_BIGS))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcIoOutcome status = current(live, &access->wait);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    if (live->kind == STYXRPC_LIVE_INTS || live->kind == STYXRPC_LIVE_BIGS)
    {
        unsigned char key[1 + STYX_LIVE_BIG_SIZE] = {0};
        size_t width = live->kind == STYXRPC_LIVE_INTS ? STYX_VARIABLE_INT_SIZE : STYX_LIVE_BIG_SIZE;
        pack_number((uint64_t)index, key + live->keyed, width);
        width += live->keyed;
        status = open_at(live->fid, ".idx", STYXRPC_FILE_WRITE, &access->wait, &access->selector);
        if (status == STYXRPC_IO_OK)
        {
            status = write_file(access->selector, key, width, &access->wait);
        }
        if (status != STYXRPC_IO_OK)
        {
            return status == STYXRPC_IO_UNCERTAIN && access->wait.stopped ? STYXRPC_IO_CANCELLED : status;
        }
    }
    return open_at(live->fid, live->kind == STYXRPC_LIVE_INT ? "." : ".val",
        writing ? STYXRPC_FILE_WRITE : STYXRPC_FILE_READ, &access->wait, &access->value);
}


/* EN: Retire the data fid before its selector. The exact binding stays open.
 * RU: Закрыть fid данных перед селектором, сохраняя основную привязку. */
static void finish(LiveAccess *access)
{
    styxrpc_vfs_control_close(access->value, &access->wait);
    styxrpc_vfs_control_close(access->selector, &access->wait);
}


/* EN/RU: See live_client.h. / См. live_client.h. */
StyxRpcIoOutcome styxrpc_live_read_until(const StyxRpcLive *live, int64_t index, StyxRpcLiveValue *out,
    StyxRpcVfsStop stop, void *state)
{
    LiveAccess access = {{stop, state, 0}, NULL, NULL};
    if (!out)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcIoOutcome status = open_value(live, index, 0, &access);
    unsigned char bytes[STYX_LIVE_STRING_FRAME + 1];
    size_t size;
    if (status == STYXRPC_IO_OK)
    {
        status = read_file(access.value, bytes, sizeof(bytes), &size, &access.wait);
    }
    finish(&access);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    StyxRpcLiveValue value = {0};
    if (live->kind == STYXRPC_LIVE_TEXT)
    {
        FVM_SRecord record;
        size_t used;
        if (fvm_srecord_read(bytes, size, STYX_LIVE_STRING_FRAME, &record, &used) != FVM_S_OK ||
            used != size || record.size > sizeof(value.text) ||
            ((record.tag != STYXRPC_VALUE_NIL || record.size) && (record.tag != STYXRPC_VALUE_STRING ||
                fvm_utf8_validate((const char *)record.payload, record.size, NULL) != FVM_UTF8_OK)))
        {
            return STYXRPC_IO_FAILED;
        }
        value.nil = record.tag == STYXRPC_VALUE_NIL;
        value.size = record.size;
        memcpy(value.text, record.payload, record.size);
    }
    else if (live->kind == STYXRPC_LIVE_BIG)
    {
        if (styxrpc_value_get_integer(STYXRPC_VALUE_BIG, bytes, size, &value.integer) != STYXRPC_OK)
        {
            return STYXRPC_IO_FAILED;
        }
    }
    else
    {
        if (size != (live->kind == STYXRPC_LIVE_BIGS ? STYX_LIVE_BIG_SIZE : STYX_VARIABLE_INT_SIZE))
        {
            return STYXRPC_IO_FAILED;
        }
        value.integer = unpack_number(bytes, size);
    }
    *out = value;
    return STYXRPC_IO_OK;
}


/* EN/RU: See live_client.h. / См. live_client.h. */
StyxRpcIoOutcome styxrpc_live_write_until(const StyxRpcLive *live, int64_t index, const StyxRpcLiveValue *value,
    StyxRpcVfsStop stop, void *state)
{
    if (!live || !value)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    unsigned char bytes[STYX_LIVE_STRING_FRAME];
    size_t size;
    if (live->kind == STYXRPC_LIVE_TEXT)
    {
        if (value->size > sizeof(value->text) || (value->nil != 0 && (value->nil != 1 || value->size)) ||
            fvm_utf8_validate(value->text, value->size, NULL) != FVM_UTF8_OK ||
            fvm_srecord_write(value->nil ? STYXRPC_VALUE_NIL : STYXRPC_VALUE_STRING,
                value->text, value->size, bytes, sizeof(bytes), &size) != FVM_S_OK)
        {
            return STYXRPC_IO_FAILED;
        }
    }
    else if (live->kind == STYXRPC_LIVE_BIG)
    {
        if (styxrpc_value_integer(STYXRPC_VALUE_BIG, value->integer, bytes, sizeof(bytes), &size) != STYXRPC_OK)
        {
            return STYXRPC_IO_FAILED;
        }
    }
    else
    {
        size = live->kind == STYXRPC_LIVE_BIGS ? STYX_LIVE_BIG_SIZE : STYX_VARIABLE_INT_SIZE;
        if (size == STYX_VARIABLE_INT_SIZE && (value->integer < INT32_MIN || value->integer > INT32_MAX))
        {
            return STYXRPC_IO_FAILED;
        }
        pack_number((uint64_t)value->integer, bytes, size);
    }
    LiveAccess access = {{stop, state, 0}, NULL, NULL};
    StyxRpcIoOutcome status = open_value(live, index, 1, &access);
    if (status == STYXRPC_IO_OK)
    {
        status = write_file(access.value, bytes, size, &access.wait);
    }
    finish(&access);
    return status;
}


/* EN/RU: See live_client.h. / См. live_client.h. */
void styxrpc_live_close(StyxRpcLive *live, int deferred)
{
    if (live)
    {
        styxrpc_vfs_close(live->fid, deferred);
        free(live);
    }
}
