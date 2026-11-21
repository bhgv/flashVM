/* Copyright (c) 2026 bhgv
 * EN: Bounded, generation-checked concurrent caller grants.
 * RU: Ограниченные параллельные контексты с проверкой поколения.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "caller_scope.h"
#include <string.h>
#include "export.h"
#include <inttypes.h>
#include <stdio.h>


/* EN/RU: See caller_scope.h. Publish only after checking duplicates and capacity.
 * См. caller_scope.h. Проверить повторы и вместимость до публикации. */
int styxcallerscopeadd(StyxCallerScopes *scopes, DVM_Provider *tree, uint32_t pid)
{
    if (!pid || !tree)
    {
        return 0;
    }
    pthread_mutex_lock(&scopes->lock);
    StyxCallerGrant *free_slot = NULL;
    for (size_t i = 0; i < STYX_CALLER_SLOTS; i++)
    {
        StyxCallerGrant *slot = &scopes->active[i];
        if (slot->tree && (slot->pid == pid || slot->tree == tree))
        {
            pthread_mutex_unlock(&scopes->lock);
            return 0;
        }
        if (!slot->tree && !free_slot)
        {
            free_slot = slot;
        }
    }
    int accepted = free_slot && !scopes->stopped && scopes->generation < UINT64_MAX / STYX_CALLER_NODES;
    if (accepted)
    {
        *free_slot = (StyxCallerGrant){.tree = tree, .pid = pid, .generation = ++scopes->generation};
    }
    pthread_mutex_unlock(&scopes->lock);
    return accepted;
}


/* EN/RU: See caller_scope.h. Existing I/O may finish through its retained cursor.
 * См. caller_scope.h. Начатый I/O может завершиться через удерживаемый курсор. */
void styxcallerscopedrop(StyxCallerScopes *scopes, DVM_Provider *tree)
{
    pthread_mutex_lock(&scopes->lock);
    for (size_t i = 0; i < STYX_CALLER_SLOTS; i++)
    {
        if (scopes->active[i].tree == tree)
        {
            scopes->active[i] = (StyxCallerGrant){0};
            break;
        }
    }
    pthread_mutex_unlock(&scopes->lock);
}


/* EN/RU: See caller_scope.h. Return slot index or -1 and avoid callbacks while locked.
 * См. caller_scope.h. Вернуть индекс либо -1. Callback под блокировкой не вызывается. */
int styxcallerscopefind(StyxCallerScopes *scopes, uint32_t pid, uint64_t generation, StyxCallerGrant *result)
{
    int index = -1;
    pthread_mutex_lock(&scopes->lock);
    for (size_t i = 0; !scopes->stopped && i < STYX_CALLER_SLOTS; i++)
    {
        StyxCallerGrant *slot = &scopes->active[i];
        if (slot->tree && (pid ? slot->pid == pid : slot->generation == generation) &&
            (!result || dvm_provider_retain(slot->tree)))
        {
            if (result)
            {
                *result = *slot;
            }
            index = (int)i;
            break;
        }
    }
    pthread_mutex_unlock(&scopes->lock);
    return index;
}


/* EN/RU: See caller_scope.h. The view snapshots ownership under the lock.
 * См. caller_scope.h. Снимок удерживает ссылки под блокировкой. */
DVM_Provider *styxcallerscopeview(StyxCallerScopes *scopes)
{
    StyxCallerGrant grants[STYX_CALLER_SLOTS] = {0};
    int valid = 1;
    pthread_mutex_lock(&scopes->lock);
    for (size_t i = 0; i < STYX_CALLER_SLOTS; i++)
    {
        StyxCallerGrant *slot = &scopes->active[i];
        if (slot->tree)
        {
            if (!dvm_provider_retain(slot->tree))
            {
                valid = 0;
                break;
            }
            grants[i] = *slot;
        }
    }
    pthread_mutex_unlock(&scopes->lock);
    DVM_View *view = valid ? dvm_view_create(NULL, NULL, NULL) : NULL;
    for (size_t i = 0; i < STYX_CALLER_SLOTS; i++)
    {
        if (view && grants[i].tree)
        {
            char path[sizeof("/.prog/4294967295")];
            snprintf(path, sizeof(path), "/.prog/%" PRIu32, grants[i].pid);
            if (!dvm_view_map(view, path, grants[i].tree, path + 1, FVM_ACCESS_READ | FVM_ACCESS_EXEC, DVM_VIEW_TREE))
            {
                dvm_view_discard(view);
                view = NULL;
            }
        }
        dvm_provider_release(grants[i].tree);
    }
    return dvm_view_freeze(view);
}


/* EN: Copy display metadata into a stat record. Name storage remains borrowed.
 * RU: Скопировать метаданные в stat. Хранилище имени заимствуется. */
void styxcallerattributes(const StyxMeta *meta, Qid qid, Dir *dir)
{
    memset(dir, 0, sizeof(*dir));
    dir->qid = qid;
    dir->mode = meta->mode | (meta->directory ? DMDIR : 0);
    dir->length = meta->length;
    dir->name = (char *)meta->name;
    dir->uid = dir->gid = dir->muid = "caller";
}


/* EN: Encode complete directory entries, advancing only after copying each entry.
 * RU: Кодировать полные записи каталога, продвигая курсор после копирования записи. */
char *styxcallerlist(StyxOpen *opened, char *bytes, ulong *count, vlong offset)
{
    if (offset < 0 || !styxdirbegin(opened, offset))
    {
        return Eperm;
    }
    ulong used = 0;
    while (used < *count)
    {
        StyxMeta meta;
        int next = styxdirpeek(opened, &meta);
        if (!next)
        {
            break;
        }
        if (next < 0)
        {
            if (!used)
            {
                return Eperm;
            }
            break;
        }
        Dir dir;
        styxcallerattributes(&meta, styxqid(meta.identity, meta.directory), &dir);
        uint size = sizeD2M(&dir);
        if (size > *count - used)
        {
            if (!used)
            {
                return Ebadarg;
            }
            break;
        }
        if (convD2M(&dir, (uchar *)bytes + used, *count - used) != size || !styxdiradvance(opened, size))
        {
            return Ebadarg;
        }
        used += size;
    }
    *count = used;
    return nil;
}
