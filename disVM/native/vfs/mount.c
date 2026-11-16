/* Copyright (c) 2026 bhgv
 * EN: Ordinary VFS mount factory over retained native client capabilities.
 * RU: Обычная VFS mount factory над удержанными native client capabilities.
 */
#include "mount_internal.h"
#include "mount_cancel.h"
#include "control/internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: See mount_internal.h; unknown textual IDs are not fabricated numeric users.
 * См. mount_internal.h; textual IDs не превращаются в вымышленные numeric users. */
void styxmountmeta(struct Mount *mount, const StyxMeta *meta, FVM_FileInfo *out)
{
    memset(out, 0, sizeof(*out));
    strcpy(out->name, meta->name);
    out->identity.low = meta->identity & 0xffffffffUL;
    out->identity.high = meta->identity >> 32;
    out->device.low = meta->device & 0xffffffffUL;
    out->device.high = meta->device >> 32;
    out->length.low = meta->length & 0xffffffffUL;
    out->length.high = meta->length >> 32;
    out->directory = meta->directory;
    out->mode = meta->mode & 0777;
    if (!(mount->access & FVM_ACCESS_READ))
    {
        out->mode &= ~0444UL;
    }
    if (!(mount->access & FVM_ACCESS_WRITE))
    {
        out->mode &= ~0222UL;
    }
    if (!(mount->access & FVM_ACCESS_EXEC))
    {
        out->mode &= ~0111UL;
    }
}


/* EN/RU: Explicit metadata query walks once and stats a separate unopened fid.
 * Явный metadata query один раз проходит путь и stat отдельного unopened fid. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    struct Mount *mount = state;
    if (mount->file_root && strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    StyxFid *fid = styxmountwalk(mount, path);
    StyxMeta meta;
    int success = fid && styxmountstat(mount->client, fid, &meta) && (!mount->file_root || !meta.directory);
    styxmountclunk(mount->client, fid);
    if (success)
    {
        styxmountmeta(mount, &meta, out);
    }
    return success ? FVM_OK : FVM_ERROR;
}


/* EN/RU: Capture a distinct fid; only Topen decides actual server permission.
 * Захватить отдельный fid; реальные server permissions решает только Topen. */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    struct Mount *mount = state;
    if (mount->file_root && strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    int how = !strcmp(mode, "r") ? 0 : !strcmp(mode, "wo") ? 1 :
        (!strcmp(mode, "rw") || !strcmp(mode, "rwo")) ? 2 : -1;
    unsigned required = how == 0 ? FVM_ACCESS_READ : how == 1 ? FVM_ACCESS_WRITE :
        FVM_ACCESS_READ|FVM_ACCESS_WRITE;
    if (how < 0 || (required & ~mount->access))
    {
        return FVM_ERROR;
    }
    struct Mounted *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return FVM_OUT_OF_MEMORY;
    }
    opened->mount = mount;
    opened->fid = styxmountwalk(mount, path);
    StyxMeta meta;
    if (!opened->fid || !styxmountstat(mount->client, opened->fid, &meta) || (mount->file_root && meta.directory))
    {
        goto failed;
    }
    opened->directory = meta.directory;
    if (meta.directory)
    {
        if (how != 0 || !(mount->access & FVM_ACCESS_EXEC))
        {
            goto failed;
        }
        opened->records = malloc(65536);
        if (!opened->records)
        {
            goto failed;
        }
    }
    if (!styxmountopen(mount->client, opened->fid, how))
    {
        goto failed;
    }
    out->native_flags = styxclientcancellable(mount->client) ?
        DVM_VFS_CANCEL_INFO | (meta.directory ? DVM_VFS_CANCEL_LIST : DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_SEEK) : 0;
    out->ops = &styxmountedops;
    out->state = opened;
    return FVM_OK;
failed:
    styxmountclunk(mount->client, opened->fid);
    free(opened->records);
    free(opened);
    return FVM_ERROR;
}


/* EN/RU: Final provider/handle reference detaches watch and consumes connection without callback wait.
 * Последняя provider/handle ссылка отсоединяет watch и поглощает connection без callback wait. */
static void destroy(void *state)
{
    struct Mount *mount = state;
    styxmountctlclose(mount->control);
    styxmountctlfree(mount->control);
    styxclientunwatch(mount->watch);
    styxmountrootretire(mount->rootlife);
    styxclientfree(mount->client);
    free(mount);
}
static const DVM_ProviderOps operations = {info, openfile, destroy, 0, NULL}; /* Directory subroot aliases need distinct retained-fid capabilities. */
static const DVM_ProviderOps fileoperations = {info, openfile, destroy, DVM_PROVIDER_STABLE_BIND, NULL}; /* Only dot maps to the immutable attached file fid. */


/* EN/RU: Consume client; create factory/health watch and optional configured native feed.
 * Clear optional control before fallible setup; event policy adopts state only on success.
 * Принять client; создать factory/health и optional feed; control очистить до setup.
 * Transport authority выбрана native host; config.state принимается только при успехе.
 * root is consumed always; directory names its kind; ownwatch selects a local health tap.
 * root принят всегда; directory задаёт тип, ownwatch включает свой health tap. */
static DVM_Provider *create(StyxClient *client, unsigned access, const StyxFeedConfig *config, StyxFeed **control, StyxMountCtl **authority, StyxFid *root, int directory, int ownwatch)
{
    if (authority)
    {
        *authority = NULL;
    }
    if (control)
    {
        *control = NULL;
    }
    if (!client || !access || (access & ~FVM_ACCESS_MASK) || (config && !(access & FVM_ACCESS_READ)))
    {
        styxmountclunk(client, root);
        styxclientfree(client);
        return NULL;
    }
    struct Mount *mount = calloc(1, sizeof(*mount));
    if (!mount)
    {
        styxmountclunk(client, root);
        styxclientfree(client);
        return NULL;
    }
    if (root)
    {
        mount->rootlife = styxmountrootlife(root);
        if (!mount->rootlife)
        {
            styxmountclunk(client, root);
            styxclientfree(client);
            free(mount);
            return NULL;
        }
    }
    mount->root = root;
    mount->client = client;
    mount->access = access;
    mount->file_root = root ? !directory : styxclientrootdirectory(client) == 0;
    DVM_Provider *provider = dvm_provider_create_native(mount->file_root ? &fileoperations : &operations, mount, styxmountcapture, root != NULL,
        styxclientcancellable(client) ? DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_LIST : 0);
    if (!provider)
    {
        destroy(mount);
        return NULL;
    }
    DVM_Changes *changes = dvm_provider_changes(provider);
    mount->watch = ownwatch ? styxmountwatch(client, changes) : NULL;
    if (ownwatch && !mount->watch)
    {
        dvm_provider_release(provider);
        return NULL;
    }
    mount->control = styxmountctlcreate(client, changes, access);
    if (!mount->control)
    {
        dvm_provider_release(provider);
        return NULL;
    }
    if (config)
    {
        mount->control->feed = styxfeedcreate(client, changes, config);
        if (!mount->control->feed)
        {
            dvm_provider_release(provider);
            return NULL;
        }
        mount->control->info.configured = 1;
        if (control)
        {
            styxfeedretain(mount->control->feed);
            *control = mount->control->feed;
        }
    }
    if (authority)
    {
        styxmountctlretain(mount->control);
        *authority = mount->control;
    }
    return provider;
}


/* EN/RU: See mount.h; ordinary mount keeps automatic connection-health observation only.
 * См. mount.h; ordinary mount сохраняет автоматическое connection-health observation. */
DVM_Provider *styxmountprovider(StyxClient *client, unsigned access)
{
    return create(client, access, NULL, NULL, NULL, NULL, 0, 1);
}


/* EN/RU: See mount.h; event endpoint/codec are explicit trusted host choices.
 * См. mount.h; event endpoint/codec явно выбирает trusted host. */
DVM_Provider *styxmountproviderfeed(StyxClient *client, unsigned access, const StyxFeedConfig *config, StyxFeed **control)
{
    if (!config)
    {
        if (control)
        {
            *control = NULL;
        }
        styxclientfree(client);
        return NULL;
    }
    return create(client, access, config, control, NULL, NULL, 0, 1);
}


/* EN/RU: See mount.h; explicit authority is separate from ordinary guest provider access.
 * См. mount.h; explicit authority отделена от ordinary guest provider access. */
DVM_Provider *styxmountprovidercontrol(StyxClient *client, unsigned access,
                                      const StyxFeedConfig *initial, StyxMountCtl **output)
{
    if (!output)
    {
        styxclientfree(client);
        return NULL;
    }
    return create(client, access, initial, NULL, output, NULL, 0, 1);
}


/* EN/RU: See mount_internal.h; capture providers inherit health/content via source journal bridge.
 * См. mount_internal.h; capture providers наследуют health/content через source journal bridge. */
DVM_Provider *styxmountroot(StyxClient *client, StyxFid *root, unsigned access, int directory, int watch)
{
    return create(client, access, NULL, NULL, NULL, root, directory, watch);
}
