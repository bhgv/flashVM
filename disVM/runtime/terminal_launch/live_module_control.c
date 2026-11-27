/* Copyright (c) 2026 bhgv
 * EN: Hold one load request on an exact Styx control fid.
 * RU: Удерживать один запрос загрузки на точном fid Styx.
 */
#include "live_modules_private.h"
#include "styxrpc/named_channel.h"
#include <inttypes.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ModuleRequest
{
    LiveModules *session; /* Borrowed until the reverse dispatcher joins. */
    LiveModule *entry; /* Owned generation lease, or NULL before write. */
    size_t offset; /* Position in the fixed-width response. */
} ModuleRequest;


/* EN/RU: See live_modules_private.h. / См. live_modules_private.h. */
int dvm_live_module_path(const char *path)
{
    size_t length = strlen(path);
    if (length <= 4 || length > STYXRPC_VFS_PATH_MAX || strcmp(path + length - 4, ".dis"))
    {
        return 0;
    }
    for (;;)
    {
        size_t part = strcspn(path, "/");
        if (!part || (part == 1 && *path == '.') ||
            (part == 2 && !memcmp(path, "..", 2)) || memchr(path, '\t', part) ||
            styxrpc_vfs_service_name(path, part))
        {
            return 0;
        }
        if (!path[part])
        {
            return 1;
        }
        path += part + 1;
    }
}


/* EN/RU: See live_modules_private.h. / См. live_modules_private.h. */
void dvm_live_module_id(char *output, uint32_t id)
{
    snprintf(output, STYXRPC_LIVE_MODULE_ID_BYTES + 1, "%08" PRIx32, id);
}


/* EN/RU: See live_modules_private.h. / См. live_modules_private.h. */
uint32_t dvm_live_module_parse(const char *input)
{
    uint32_t value = 0;
    for (size_t i = 0; i < STYXRPC_LIVE_MODULE_ID_BYTES; i++)
    {
        const char *digits = "0123456789abcdef";
        const char *digit = input[i] ? strchr(digits, input[i]) : NULL;
        if (!digit)
        {
            return 0;
        }
        value = (value << 4) | (unsigned)(digit - digits);
    }
    return (!input[STYXRPC_LIVE_MODULE_ID_BYTES] ||
        input[STYXRPC_LIVE_MODULE_ID_BYTES] == '/') ? value : 0;
}


/* EN: Admit one counted path and signal the listener without blocking Styx.
 * RU: Принять путь с длиной и пробудить приёмник без блокировки Styx. */
static FVM_Result request_write(void *state, const void *bytes, size_t size)
{
    ModuleRequest *request = state;
    LiveModules *session = request->session;
    char path[STYXRPC_VFS_PATH_MAX + 1];
    if (request->entry || request->offset || !size || size >= sizeof(path) || memchr(bytes, '\0', size))
    {
        return FVM_INVALID_ARGUMENT;
    }
    memcpy(path, bytes, size);
    path[size] = '\0';
    if (!dvm_live_module_path(path))
    {
        return FVM_INVALID_ARGUMENT;
    }
    pthread_mutex_lock(&session->lock);
    LiveModule *entry = NULL;
    for (size_t i = 0; !session->stopped && session->last < UINT32_MAX &&
        i < DVM_LIVE_MODULE_LIMIT; ++i)
    {
        if (session->entries[i].phase == LIVE_UNUSED)
        {
            entry = &session->entries[i];
            break;
        }
    }
    if (!entry)
    {
        pthread_mutex_unlock(&session->lock);
        return FVM_UNSUPPORTED;
    }
    request->entry = entry;
    entry->generation = ++session->last;
    entry->leased = 1;
    entry->breaker = -1;
    entry->path[0] = '/';
    memcpy(entry->path + 1, path, size + 1);
    entry->phase = LIVE_QUEUED;
    pthread_cond_broadcast(&session->changed);
    pthread_mutex_unlock(&session->lock);
    return FVM_OK;
}


/* EN: Return only the exact committed generation. Reads can be fragmented.
 * RU: Вернуть только точное опубликованное поколение. Чтение можно дробить. */
static FVM_Result request_read(void *state, void *bytes, size_t size, size_t *used)
{
    ModuleRequest *request = state;
    *used = 0;
    if (!request->entry)
    {
        return FVM_ERROR;
    }
    char id[STYXRPC_LIVE_MODULE_ID_BYTES + 1];
    pthread_mutex_lock(&request->session->lock);
    LiveModulePhase phase = request->session->stopped ? LIVE_FAILED : request->entry->phase;
    uint32_t generation = request->entry->generation;
    pthread_mutex_unlock(&request->session->lock);
    if (phase != LIVE_READY)
    {
        return phase == LIVE_FAILED ? FVM_ERROR : FVM_OK;
    }
    dvm_live_module_id(id, generation);
    size_t left = STYXRPC_LIVE_MODULE_ID_BYTES - request->offset;
    *used = size < left ? size : left;
    memcpy(bytes, id + request->offset, *used);
    request->offset += *used;
    return FVM_OK;
}


/* EN: Reposition the reply cursor, which spans only the generation ID.
 * RU: Установить позицию чтения в пределах идентификатора поколения. */
static FVM_Result request_seek(void *state, long offset, int origin, long *position)
{
    ModuleRequest *request = state;
    if (origin != FVM_FILE_SEEK_START || offset < 0 || offset > STYXRPC_LIVE_MODULE_ID_BYTES)
    {
        return FVM_INVALID_ARGUMENT;
    }
    request->offset = (size_t)offset;
    *position = offset;
    return FVM_OK;
}


/* EN: Describe the exact control entry. / RU: Описать точный узел управления. */
static FVM_Result request_info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, STYXRPC_LIVE_MODULE_REQUEST);
    out->mode = 0666;
    return FVM_OK;
}


/* EN: Revoke this generation and let the resident listener collect it after exit.
 * RU: Отозвать поколение. Приёмник освободит его после выхода владельца. */
static void request_close(void *state)
{
    ModuleRequest *request = state;
    LiveModules *session = request->session;
    LiveModule *entry = request->entry;
    if (entry)
    {
        pthread_mutex_lock(&session->lock);
        entry->leased = 0;
        entry->phase = LIVE_FAILED;
        if (entry->breaker >= 0)
        {
            shutdown(entry->breaker, SHUT_RDWR);
        }
        pthread_cond_broadcast(&session->changed);
        pthread_mutex_unlock(&session->lock);
    }
    free(request);
}


/* EN: Allocate one independent cursor. / RU: Выделить независимый курсор. */
static FVM_Result request_open(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    static const DVM_VfsOps ops = {.read = request_read, .write = request_write,
        .seek = request_seek, .close = request_close};
    if (strcmp(path, ".") || (strcmp(mode, "rw") && strcmp(mode, "rwo")))
    {
        return FVM_UNSUPPORTED;
    }
    ModuleRequest *request = calloc(1, sizeof(*request));
    if (!request)
    {
        return FVM_OUT_OF_MEMORY;
    }
    request->session = state;
    *out = (DVM_VfsHandle){.ops = &ops, .state = request};
    return FVM_OK;
}


/* EN: The session owns the borrowed state beyond all provider callbacks.
 * RU: Сессия удерживает заимствованное состояние дольше всех колбэков. */
static void request_destroy(void *state)
{
    (void)state;
}


/* EN/RU: See live_modules_private.h. / См. live_modules_private.h. */
DVM_Provider *dvm_live_module_control(LiveModules *session)
{
    static const DVM_ProviderOps ops = {.info = request_info, .open = request_open, .destroy = request_destroy};
    return dvm_provider_create(&ops, session);
}
