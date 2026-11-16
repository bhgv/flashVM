/* Copyright (c) 2026 bhgv
 * EN: Retained remote root fids for ordinary mount and native per-node bind capture.
 * RU: Удерживаемые remote root fids для ordinary mount и native per-node bind capture.
 */
#include "mount_internal.h"
#include <string.h>


/* EN/RU: See mount_internal.h; rooted mounts never restart at client attach fid1.
 * См. mount_internal.h; rooted mounts не возвращаются к client attach fid1. */
StyxFid *styxmountwalk(struct Mount *mount, const char *relative)
{
    return mount->root ? styxclientwalkfrom(mount->root, relative) : styxclientwalk(mount->client, relative);
}


/* EN/RU: Consume owned client/root after stat; failure does not publish a partial provider.
 * Поглотить owned client/root после stat; failure не публикует partial provider. */
static DVM_Provider *create(StyxClient *client, StyxFid *root, unsigned access, int watch)
{
    StyxMeta meta;
    if (!root || !styxclientstat(root, &meta))
    {
        styxclientclunk(root);
        styxclientfree(client);
        return NULL;
    }
    return styxmountroot(client, root, access, meta.directory, watch);
}


/* EN/RU: See mount_internal.h; source provider is pinned across native capture I/O.
 * См. mount_internal.h; source provider удержан на время native capture I/O. */
DVM_Provider *styxmountcapture(void *state, const char *relative)
{
    struct Mount *mount = state;
    if ((mount->file_root && strcmp(relative, ".")) || !styxclientretain(mount->client))
    {
        return NULL;
    }
    return create(mount->client, styxmountwalk(mount, relative), mount->access, 0);
}


/* EN/RU: See mount.h; independent public roots install their own automatic health observation.
 * См. mount.h; independent public roots устанавливают своё automatic health observation. */
DVM_Provider *styxmountproviderat(StyxClient *client, const char *relative, unsigned access)
{
    if (!access || (access & ~FVM_ACCESS_MASK))
    {
        styxclientfree(client);
        return NULL;
    }
    return create(client, styxclientwalk(client, relative), access, 1);
}
