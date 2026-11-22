/* Copyright (c) 2026 bhgv
 * EN: Actual VFS resource service backed by a bounded native read-only provider, including refusal.
 * RU: Настоящий VFS resource service над ограниченным native read-only провайдером, включая отказ.
 */
#include "service.h"
#include "resource.h"
#include "protocol.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: One independently owned source cursor, consumed by close on the VFS service owner.
 * Отдельный source cursor, close поглощает его на owner VFS service.
 */
typedef struct Cursor
{
    size_t offset; /* Confirmed resource bytes already returned. */
} Cursor;
static const unsigned char content[] = {0, 128, 255, 0, 'x'}; /* Independent binary resource, not executable SWF. */


/* EN/RU: Describe known native fixture nodes; paths/output borrowed, no host filesystem fallback.
 * Описать известные native nodes fixture, path/output заимствованы, без host fallback.
 */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    if (strcmp(path, ".") && strcmp(path, "ok.swf") && strcmp(path, "denied.swf"))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, path);
    out->directory = !strcmp(path, ".");
    out->mode = out->directory ? 0555 : state || !strcmp(path, "denied.swf") ? 0 : 0444;
    out->identity.low = out->directory ? 1 : 2;
    out->length.low = sizeof(content);
    return FVM_OK;
}


/* EN/RU: Return a precise bounded source prefix, bytes/count borrowed; no allocation or VM entry.
 * Вернуть точный ограниченный prefix, bytes/count заимствованы, без allocation/VM.
 */
static FVM_Result readbytes(void *state, void *bytes, size_t size, size_t *received)
{
    Cursor *cursor = state; /* Service-owned independent read cursor. */
    size_t count = sizeof(content) - cursor->offset; /* Remaining exact bytes. */
#if defined(STYX_SERVER_CASE) && STYX_SERVER_CASE == 1
    if (count > 1)
    {
        count = 1;
    }
#endif
    if (count > size)
    {
        count = size;
    }
    memcpy(bytes, content + cursor->offset, count);
    cursor->offset += count;
    *received = count;
    return FVM_OK;
}


/* EN/RU: Consume one native cursor, NULL harmless. / Освободить один native cursor, NULL допустим.
 */
static void closebytes(void *state)
{
    free(state);
}
static const DVM_VfsOps stream = {.read = readbytes, .close = closebytes}; /* Read-only native source. */


/* EN/RU: Open only the permitted source for reading; denial is final even for an existing path.
 * Открыть только разрешённый source для чтения, существующий denied path всё равно запрещён.
 */
static FVM_Result openbytes(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    if (state || strcmp(path, "ok.swf") || strcmp(mode, "r"))
    {
        return FVM_ERROR;
    }
    Cursor *cursor = calloc(1, sizeof(*cursor)); /* New exclusive native source owner. */
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    out->ops = &stream;
    out->state = cursor;
    return FVM_OK;
}


/* EN/RU: Stateless fixture provider has no owned storage; mandatory lifecycle callback does nothing.
 * Stateless provider fixture не владеет памятью; обязательный lifecycle callback ничего не освобождает.
 */
static void destroy(void *state)
{
    (void)state;
}
static const DVM_ProviderOps factory = {info, openbytes, destroy, DVM_PROVIDER_STABLE_BIND, NULL}; /* Immutable fixture policy. */


/* EN/RU: Return an owned live prepared namespace/NULL with no accessible native fallback file.
 * Вернуть owned live prepared namespace/NULL без доступного native fallback file.
 */
DVM_Namespace *exchangegroup(void)
{
    DVM_Namespace *group = dvm_namespace_create("/unibus-fixture-no-native-root", "/", "/"); /* Private namespace owner. */
    DVM_Provider *root = dvm_provider_create_capture(&factory, NULL, NULL, 1); /* Fixed permitted native provider. */
    DVM_NamespacePlan target; /* Copied mount target, never a wire pointer. */
    int mounted = group && root && dvm_namespace_plan(group, "/", &target) &&
        dvm_namespace_mount_provider(group, &target, root, "/fixture-provider") == 1;
    dvm_provider_release(root);
    if (!mounted)
    {
        dvm_namespace_release(group);
        return NULL;
    }
    return group;
}


/* EN/RU: Retain the live prepared namespace in an owned actual service, preserving prior fixture API.
 * Удержать живой prepared namespace настоящим service, сохраняя прежний fixture API.
 */
DVM_Provider *exchangeprovider(void)
{
    DVM_Namespace *group = exchangegroup(); /* Owned fixture root. */
    DVM_Provider *service = group ? styxmovieservice(group) : NULL; /* Output independent of caller group ref. */
    dvm_namespace_release(group);
    return service;
}


/* EN/RU: Replace the live root policy after server creation, before a fixture-gated request arrives.
 * Заменить policy живого root после создания server, до запроса через fixture gate.
 */
int exchangegroupdeny(DVM_Namespace *group)
{
    static int denied; /* Stateless non-NULL policy marker, no owned allocation. */
    DVM_Provider *root = dvm_provider_create_capture(&factory, &denied, NULL, 1); /* New live root policy. */
    DVM_NamespacePlan target; /* Current group revision, no frozen server snapshot. */
    int changed = root && dvm_namespace_plan(group, "/", &target) &&
        dvm_namespace_mount_provider(group, &target, root, "/fixture-denied") == 1;
    dvm_provider_release(root);
    return changed;
}


/* EN/RU: Execute one existing request through the real VFS service; emit borrowed checked reply bodies.
 * All service/namespace/source owners retire before return, no private filesystem protocol.
 * Исполнить запрос настоящим VFS service; emit получает checked bodies, все owners освобождаются.
 */
int exchangereply(const void *request, size_t length, void (*emit)(void *, const void *, size_t), void *context)
{
    DVM_Provider *service = exchangeprovider(); /* Owned actual namespace-backed service. */
    DVM_VfsHandle stream = {0}; /* Exclusive native service open. */
    unsigned char bytes[MOVIE_FRAME]; /* One original service S-record. */
    size_t size; /* Current encoded/read extent. */
    int complete = 0; /* Must observe the real service's terminal END. */
    int valid = service && dvm_provider_open(service, ".", "rw", &stream) == FVM_OK &&
        movie_binding(bytes, &size) && dvm_vfs_write(&stream, bytes, size) == FVM_OK &&
        fvm_srecord_write(MOVIE_TAG, request, length, bytes, sizeof(bytes), &size) == FVM_S_OK &&
        dvm_vfs_write(&stream, bytes, size) == FVM_OK;
    while (valid && !complete)
    {
        FVM_SRecord record; /* Borrowed original service record until next read. */
        size_t consumed; /* Exact checked frame length. */
        valid = dvm_vfs_read(&stream, bytes, sizeof(bytes), &size) == FVM_OK && size &&
            fvm_srecord_read(bytes, size, MOVIE_FRAME, &record, &consumed) == FVM_S_OK && consumed == size;
        if (valid && record.tag == MOVIE_TAG)
        {
            emit(context, record.payload, record.size);
            complete = record.size && record.payload[0] == MOVIE_END;
        }
        else if (valid && record.tag != 0)
        {
            valid = 0;
        }
    }
    dvm_vfs_close(&stream);
    dvm_provider_release(service);
    return valid && complete;
}
