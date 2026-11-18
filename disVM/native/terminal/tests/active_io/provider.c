/* Copyright (c) 2026 bhgv
 * EN: Actual VFS source blocks metadata/open/read/close until its native scope is cancelled.
 * RU: Настоящий VFS source ждёт native cancellation в metadata/open/read/close.
 */
#include "namespace.h"
#include "provider.h"
#include "resource.h"
#include "gate.h"
#include <stdlib.h>
#include <string.h>
#ifndef STYX_ACTIVE_IO_CASE
#define STYX_ACTIVE_IO_CASE 0 /* Block resource metadata. */
#endif
static unsigned opened; /* Atomic successful native cursor creations. */
static unsigned closed; /* Atomic consuming native cursor closes. */


/* EN/RU: Independent source cursor, solely owned by one VFS handle until consuming close.
 * Независимый source cursor принадлежит одному VFS handle до consuming close.
 */
typedef struct Cursor
{
    size_t offset; /* Confirmed source prefix, owner only. */
} Cursor;
static const unsigned char content[] = {0, 128, 255, 0, 'x'}; /* Existing independent binary oracle. */


/* EN/RU: Describe root/resource or block independent metadata; path/out borrowed through return.
 * Описать root/resource либо ждать independent metadata; path/out borrowed до return.
 */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    if (strcmp(path, ".") && strcmp(path, "ok.swf") && strcmp(path, "other"))
    {
        return FVM_NOT_FOUND;
    }
    if ((!strcmp(path, "other") && !activegate(1)) ||
        (!strcmp(path, "ok.swf") && STYX_ACTIVE_IO_CASE % 4 == 0 && !activegate(0)))
    {
        return FVM_ERROR;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, path);
    out->directory = !strcmp(path, ".");
    out->mode = out->directory ? 0555 : 0444;
    out->identity.low = out->directory ? 1 : !strcmp(path, "other") ? 3 : 2;
    out->length.low = sizeof(content);
    return FVM_OK;
}


/* EN/RU: Return exact bounded bytes, or wait for native cancellation before any read effect.
 * Вернуть точные bounded bytes либо ждать native cancellation до read effects.
 */
static FVM_Result reading(void *state, void *bytes, size_t size, size_t *received)
{
    Cursor *cursor = state; /* Exclusive source cursor. */
    *received = 0;
    if (STYX_ACTIVE_IO_CASE % 4 == 2 && !activegate(0))
    {
        return FVM_ERROR;
    }
    size_t count = sizeof(content) - cursor->offset; /* Exact available prefix. */
    if (count > size)
    {
        count = size;
    }
    memcpy(bytes, content + cursor->offset, count);
    cursor->offset += count;
    *received = count;
    return FVM_OK;
}


/* EN/RU: Always consume one cursor, even after pre-cancel; close gate is explicitly cancellation-capable.
 * Всегда поглотить cursor, включая pre-cancel; close gate явно cancellation-capable.
 */
static void closing(void *state)
{
    if (STYX_ACTIVE_IO_CASE % 4 == 3)
    {
        (void)activegate(0);
    }
    __atomic_add_fetch(&closed, 1, __ATOMIC_RELEASE);
    free(state);
}
static const DVM_VfsOps stream = {NULL, reading, NULL, NULL, NULL, closing, NULL, NULL, NULL}; /* Scoped native read/close. */


/* EN/RU: Open exactly one read source, refusing cancelled open without transferring a cursor.
 * Открыть один read source; cancelled open не передаёт cursor.
 */
static FVM_Result opening(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)state;
    if (strcmp(path, "ok.swf") || strcmp(mode, "r") ||
        (STYX_ACTIVE_IO_CASE % 4 == 1 && !activegate(0)))
    {
        return FVM_ERROR;
    }
    Cursor *cursor = calloc(1, sizeof(*cursor)); /* New independently owned native source. */
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    out->ops = &stream;
    out->state = cursor;
    out->native_flags = DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_CLOSE;
    __atomic_add_fetch(&opened, 1, __ATOMIC_RELEASE);
    return FVM_OK;
}


/* EN/RU: Static native factory state has no storage to release; no provider or VM reentry.
 * Static native factory state не владеет storage, без provider/VM reentry.
 */
static void destroy(void *state)
{
    (void)state;
}
static const DVM_ProviderOps factory = {info, opening, destroy, DVM_PROVIDER_STABLE_BIND, NULL}; /* Read-only scoped factory. */


/* EN/RU: Owned isolated namespace/NULL; publish borrowed provider root, retaining factory on success.
 * Свой isolated namespace/NULL; опубликовать borrowed provider root с retain при успехе.
 */
DVM_Namespace *activegroup(DVM_Provider *root)
{
    DVM_Namespace *group = dvm_namespace_create("/active-io-no-native-fallback", "/", "/"); /* No accessible host fallback. */
    DVM_NamespacePlan target; /* Current namespace plan. */
    if (!group || !dvm_namespace_plan(group, "/", &target) ||
        dvm_namespace_mount_provider(group, &target, root, "/active-provider") != 1)
    {
        dvm_namespace_release(group);
        return NULL;
    }
    return group;
}


/* EN/RU: Return owned trusted provider/NULL with explicit PATH cancellation contract and immutable root.
 * Свой trusted provider/NULL с явным PATH cancellation contract и immutable root.
 */
DVM_Provider *activeprovider(void)
{
    return dvm_provider_create_native(&factory, NULL, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
}


/* EN/RU: After server join, verify every published cursor was closed exactly once; 1 balanced/0 failed.
 * После server join проверить ровно один close каждого cursor; 1 balanced/0 failed.
 */
int activebalanced(void)
{
    unsigned expected = STYX_ACTIVE_IO_CASE % 4 >= 2 ? 1 : 0; /* Metadata/open cancellation never publishes a source cursor. */
    return __atomic_load_n(&opened, __ATOMIC_ACQUIRE) == expected &&
        __atomic_load_n(&closed, __ATOMIC_ACQUIRE) == expected;
}
