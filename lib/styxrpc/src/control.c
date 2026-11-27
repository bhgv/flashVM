/* Copyright (c) 2026 bhgv
 * EN: Framed single upload and owner command/status transactions over ordinary fids.
 * RU: Одна кадровая загрузка, команды и состояние владельца через обычные fid.
 */
#include "styxrpc/control.h"
#include "control_internal.h"
#include <stdlib.h>
#include <string.h>

struct StyxRpcControl
{
    StyxRpcControlSpec spec; /* Copied owner callbacks. */
    int (*extra)(void *, const char *, size_t); /* Optional counted command on the same owner. */
    int upload_open;        /* An upload fid owns the one staging slot. */
};

typedef struct ControlFile
{
    StyxRpcControl *control; /* Borrowed through close. */
    uint64_t id;             /* Exact held node. */
    unsigned access;        /* Negotiated read/write bits. */
    size_t staged;           /* Bytes acknowledged on this write cursor. */
    size_t length;           /* Upload payload or status snapshot length. */
    unsigned char prefix[STYXRPC_UPLOAD_PREFIX]; /* Count bytes, staged across writes. */
    unsigned char *data;     /* Owned payload staging or status snapshot. */
    int finished;           /* This fid can no longer commit a second command or upload. */
    int sampled;            /* Status snapshot has been acquired. */
} ControlFile;

static const StyxRpcEntry entries[] = {
    {CONTROL_ROOT, "/", 0555, 1}, {CONTROL_UPLOAD, ".upload", 0222, 0}, {CONTROL_STATE, ".state", 0666, 0}
}; /* Fixed public control nodes. */


/* EN/RU: See styxrpc/control.h. / См. styxrpc/control.h. */
StyxRpcStatus styxrpc_control_create(const StyxRpcControlSpec *spec, StyxRpcControl **out)
{
    if (!spec || !out || !spec->upload || !spec->upload_limit || !spec->command || !spec->status ||
        (uint64_t)spec->upload_limit + STYXRPC_UPLOAD_PREFIX > SIZE_MAX)
    {
        return STYXRPC_INVALID;
    }
    StyxRpcControl *control = calloc(1, sizeof(*control));
    if (!control)
    {
        return STYXRPC_INVALID;
    }
    control->spec = *spec;
    *out = control;
    return STYXRPC_OK;
}


/* EN/RU: See styxrpc/control.h. / См. styxrpc/control.h. */
void styxrpc_control_destroy(StyxRpcControl *control)
{
    free(control);
}


/* EN/RU: See styxrpc/control.h. / См. styxrpc/control.h. */
void styxrpc_control_set_extension(StyxRpcControl *control, int (*command)(void *, const char *, size_t))
{
    if (control)
    {
        control->extra = command;
    }
}


/* EN: Enumerate immediate control files below the root.
 * RU: Перечислить непосредственные файлы управления под корнем. */
static const StyxRpcEntry *enumerate(void *unused, uint64_t directory, size_t index)
{
    (void)unused;
    return directory == CONTROL_ROOT && index < CONTROL_STATE ? entries + index + 1 : NULL;
}


/* EN: Copy stable metadata for a root or file node.
 * RU: Скопировать устойчивые метаданные корня или файла. */
static int describe(void *unused, uint64_t id, StyxRpcNode *node)
{
    (void)unused;
    if (id > CONTROL_STATE)
    {
        return 0;
    }
    *node = (StyxRpcNode){.entry = entries[id], .parent = CONTROL_ROOT};
    return 1;
}


/* EN: Allow one upload fid or independent state snapshots. Publish only complete open state.
 * RU: Допустить один fid загрузки либо независимые снимки состояния. Выдавать только готовое состояние. */
static int open_file(void *context, uint64_t id, unsigned access, void **cookie)
{
    StyxRpcControl *control = context;
    if (id < CONTROL_UPLOAD || id > CONTROL_STATE || !access || (access & ~(STYXRPC_READ | STYXRPC_WRITE)) ||
        (id == CONTROL_UPLOAD && (access != STYXRPC_WRITE || control->upload_open)))
    {
        return 0;
    }
    ControlFile *file = calloc(1, sizeof(*file));
    if (!file)
    {
        return 0;
    }
    file->control = control;
    file->id = id;
    file->access = access;
    if (id == CONTROL_UPLOAD)
    {
        control->upload_open = 1;
    }
    *cookie = file;
    return 1;
}


/* EN: Sample text once and serve the immutable bytes according to the read offset.
 * RU: Получить снимок текста один раз и обслуживать его байты по смещению. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *used)
{
    ControlFile *file = cookie;
    if (!(file->access & STYXRPC_READ))
    {
        return 0;
    }
    if (!file->sampled)
    {
        file->data = malloc(STYXRPC_STATE_LIMIT);
        if (!file->data || file->control->spec.status(file->control->spec.context,
                (char *)file->data, STYXRPC_STATE_LIMIT, &file->length) != 1 || file->length > STYXRPC_STATE_LIMIT)
        {
            free(file->data);
            file->data = NULL;
            return 0;
        }
        file->sampled = 1;
    }
    size_t remaining = offset < file->length ? file->length - (size_t)offset : 0;
    *used = remaining < capacity ? remaining : capacity;
    if (*used)
    {
        memcpy(bytes, file->data + (size_t)offset, *used);
    }
    return 1;
}


/* EN: Accumulate one bounded counted body and commit at the final acknowledged write.
 * RU: Собрать одно тело с проверенной длиной и передать его при последней записи. */
static int upload(ControlFile *file, const unsigned char *bytes, size_t size)
{
    if (file->staged < sizeof(file->prefix))
    {
        size_t take = sizeof(file->prefix) - file->staged;
        if (take > size)
        {
            take = size;
        }
        memcpy(file->prefix + file->staged, bytes, take);
        file->staged += take;
        bytes += take;
        size -= take;
        if (file->staged < sizeof(file->prefix))
        {
            return 1;
        }
        uint32_t length = 0;
        for (size_t i = 0; i < sizeof(file->prefix); ++i)
        {
            length = (length << 8) | file->prefix[i];
        }
        if (!length || length > file->control->spec.upload_limit || size > length || !(file->data = malloc(length)))
        {
            return 0;
        }
        file->length = length;
    }
    size_t payload = file->staged - sizeof(file->prefix);
    if (size > file->length - payload)
    {
        return 0;
    }
    memcpy(file->data + payload, bytes, size);
    file->staged += size;
    if (payload + size == file->length)
    {
        file->finished = 1;
        int accepted = file->control->spec.upload(file->control->spec.context, file->data, file->length) == 1;
        free(file->data);
        file->data = NULL;
        return accepted;
    }
    return 1;
}


/* EN: Parse one exact ASCII word plus optional newline, and invoke the owner once.
 * RU: Разобрать одно точное слово ASCII с необязательным переводом строки и вызвать владельца. */
static int command(StyxRpcControl *control, const char *bytes, size_t size)
{
    static const char *const words[] = {"start", "pause", "restart", "debug"}; /* Command enum order. */
    if (size && bytes[size - 1] == '\n')
    {
        --size;
    }
    for (size_t i = 0; i < sizeof(words) / sizeof(*words); ++i)
    {
        if (size == strlen(words[i]) && !memcmp(bytes, words[i], size))
        {
            return control->spec.command(control->spec.context, (StyxRpcControlCommand)i) == 1;
        }
    }
    return control->extra && control->extra(control->spec.context, bytes, size) == 1;
}


/* EN: Validate contiguity and map one write to its exact open control node.
 * RU: Проверить последовательность и передать запись её точному открытому узлу. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t size, size_t *used)
{
    ControlFile *file = cookie;
    if (!(file->access & STYXRPC_WRITE) || file->finished || offset != file->staged || !size)
    {
        return 0;
    }
    int ok;
    if (file->id == CONTROL_UPLOAD)
    {
        ok = upload(file, bytes, size);
    }
    else
    {
        file->finished = 1;
        ok = command(file->control, bytes, size);
    }
    if (!ok)
    {
        file->finished = 1;
        return 0;
    }
    *used = size;
    return 1;
}


/* EN: Retire unconfirmed uploads and snapshots without invoking an owner callback.
 * RU: Отбросить неподтверждённые загрузки и снимки без вызова владельца. */
static void close_file(void *cookie)
{
    ControlFile *file = cookie;
    if (file->id == CONTROL_UPLOAD)
    {
        file->control->upload_open = 0;
    }
    free(file->data);
    free(file);
}

const StyxRpcTree styxrpc_control_tree = {{enumerate, open_file, read_file, write_file, close_file}, describe}; /* Ordinary Styx control tree. */
