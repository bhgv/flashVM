/* Copyright (c) 2026 bhgv
 * EN: Ordinary VFS directory projects one granted control file and current movie generation.
 * RU: Обычный VFS каталог выдаёт granted control file и current movie generation.
 */
#include "offer.h"
#include "cancel.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct OfferRoot
{
    DVM_MovieOffer *source; /* Owned trusted per-job scope. */
    DVM_Provider *control; /* Owned fixed single-file control grant. */
    DVM_Changes *journal; /* Owned source-journal pin through subscription disposal. */
    DVM_NoticeTap *tap; /* Owned push forwarding; callback state is an independent journal pin. */
} OfferRoot;


typedef struct OpenNode
{
    DVM_VfsHandle inner; /* Owned selected underlying file, never a host pathname. */
    DVM_VfsOps ops; /* Exact forwarded native operation availability. */
    char name[DVM_MOVIE_NAME]; /* Projected held-open basename. */
    uint64_t identity; /* Stable projection for held-open metadata. */
} OpenNode;


typedef struct Directory
{
    OfferRoot *root; /* Borrowed through provider handle's independent factory pin. */
    int movies; /* Zero root, one movie-generation directory. */
    int done; /* This cursor's one bounded listing has been consumed. */
} Directory;


/* EN/RU: Project native scalar identity without changing effective permissions or resource content.
 * Проецировать scalar identity без изменения effective permissions/content. */
static void identity(FVM_FileInfo *info, uint64_t value)
{
    info->identity.high = (unsigned long)(value >> 32);
    info->identity.low = (unsigned long)(value & 0xffffffffUL);
    info->device.high = info->device.low = 0;
}


/* EN/RU: Select an owned exact file; canonical generation text is verified inside the source lock.
 * Выбрать owned exact file; canonical generation проверяется под source lock. */
static DVM_Provider *selectnode(OfferRoot *root, const char *path, uint64_t *id)
{
    if (!strcmp(path, "ctl"))
    {
        *id = STYX_MOVIE_CONTROL_ID;
        return dvm_provider_retain(root->control) ? root->control : NULL;
    }
    DVM_Provider *provider = dvm_movie_offer_acquire(root->source, path);
    if (provider)
    {
        *id = strtoull(path+6, NULL, 10);
    }
    return provider;
}


/* EN/RU: Describe live scoped nodes, without inventing native paths or caching resource rights.
 * Описать live scoped nodes без native paths и кэширования resource rights. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    OfferRoot *root = state;
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    if (!strcmp(path, ".") || !strcmp(path, "movie"))
    {
        memset(out, 0, sizeof(*out));
        out->directory = 1;
        out->mode = 0555;
        strcpy(out->name, !strcmp(path, ".") ? "terminal" : "movie");
        identity(out, !strcmp(path, ".") ? STYX_MOVIE_ROOT_ID : STYX_MOVIE_DIRECTORY_ID);
        return FVM_OK;
    }
    uint64_t id;
    DVM_Provider *node = selectnode(root, path, &id);
    if (!node)
    {
        return FVM_NOT_FOUND;
    }
    FVM_Result result = dvm_provider_info(node, ".", out);
    dvm_provider_release(node);
    if (result == FVM_OK)
    {
        identity(out, id);
        const char *slash = strrchr(path, '/');
        strcpy(out->name, slash ? slash+1 : path);
    }
    return result;
}


/* EN/RU: Keep held-open identity consistent with the projected walk, while querying actual metadata.
 * Сохранять identity held-open объекта, запрашивая actual metadata. */
static FVM_Result nodeinfo(void *state, FVM_FileInfo *out)
{
    OpenNode *node = state;
    FVM_Result result = dvm_vfs_info(&node->inner, out);
    if (result == FVM_OK)
    {
        identity(out, node->identity);
        strcpy(out->name, node->name);
    }
    return result;
}


/* EN/RU: Forward one precise acknowledged read; no retry, native bytes borrowed through return.
 * Передать один precise read без retry; bytes borrowed до return. */
static void readnode(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    *out = dvm_vfs_read_chunk(&((OpenNode *)state)->inner, bytes, size);
}


/* EN/RU: Forward one precise write without replay or widening mode authority.
 * Передать precise write без replay/расширения mode authority. */
static void writenode(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    *out = dvm_vfs_write_chunk(&((OpenNode *)state)->inner, bytes, size);
}


/* EN/RU: Forward seek only when the underlying endpoint actually supports it.
 * Передать seek только при фактической поддержке underlying endpoint. */
static FVM_Result seeknode(void *state, long offset, int origin, long *position)
{
    return dvm_vfs_seek(&((OpenNode *)state)->inner, offset, origin, position);
}


/* EN/RU: Close an independently held source even after its generation was retired.
 * Закрыть independent source даже после retirement его generation. */
static void closenode(void *state)
{
    OpenNode *node = state;
    dvm_vfs_close(&node->inner);
    free(node);
}


/* EN/RU: Query exclusive close capability without I/O or target execution.
 * Проверить exclusive close capability без I/O/target execution. */
static int closeready(void *state)
{
    return dvm_vfs_close_cancellable(&((OpenNode *)state)->inner);
}


/* EN/RU: Borrow directory cursor through its provider pin; return live directory metadata.
 * Заимствовать directory cursor под provider pin; вернуть live metadata. */
static FVM_Result dirinfo(void *state, FVM_FileInfo *out)
{
    Directory *directory = state;
    return info(directory->root, directory->movies ? "movie" : ".", out);
}


/* EN/RU: Return one owned bounded metadata batch, then EOF; each new open sees current generation.
 * Вернуть owned bounded batch, затем EOF; новый open видит current generation. */
static FVM_Result list(void *state, FVM_FileInfo **entries, size_t *count)
{
    Directory *directory = state;
    *entries = NULL;
    *count = 0;
    if (directory->done)
    {
        return FVM_OK;
    }
    FVM_FileInfo *batch = calloc(2, sizeof(*batch));
    if (!batch)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (!directory->movies)
    {
        if (info(directory->root, "ctl", batch) != FVM_OK ||
            info(directory->root, "movie", batch+1) != FVM_OK)
        {
            free(batch);
            return FVM_ERROR;
        }
        *count = 2;
    }
    else
    {
        DVM_MovieOfferInfo current;
        char name[DVM_MOVIE_NAME];
        if (dvm_movie_offer_info(directory->root->source, &current) && current.ready &&
            dvm_movie_offer_name(current.generation, name) &&
            info(directory->root, name, batch) == FVM_OK)
        {
            *count = 1;
        }
    }
    directory->done = 1;
    if (*count)
    {
        *entries = batch;
    }
    else
    {
        free(batch);
    }
    return FVM_OK;
}


/* EN/RU: Retire one native directory cursor; no provider or source I/O.
 * Завершить directory cursor без provider/source I/O. */
static void closedir(void *state)
{
    free(state);
}


/* EN/RU: Invalidate a listing after publication failure; never resume a partial directory batch.
 * Сделать listing invalid после publication failure; partial batch не возобновлять. */
static void abortdir(void *state)
{
    ((Directory *)state)->done = 1;
}
static const DVM_VfsOps directory_ops = {dirinfo, NULL, NULL, NULL, list, closedir, abortdir, NULL, NULL}; /* Owned cursor. */


/* EN/RU: Open exact granted nodes/directories; actual underlying operation decides file access.
 * Открыть exact granted nodes/directories; file access решает underlying operation. */
static FVM_Result openroot(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    OfferRoot *root = state;
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    if (!strcmp(path, ".") || !strcmp(path, "movie"))
    {
        if (strcmp(mode, "r"))
        {
            return FVM_ERROR;
        }
        Directory *directory = calloc(1, sizeof(*directory));
        if (!directory)
        {
            return FVM_OUT_OF_MEMORY;
        }
        directory->root = root;
        directory->movies = !strcmp(path, "movie");
        out->state = directory;
        out->ops = &directory_ops;
        out->native_flags = DVM_VFS_CANCEL_INFO | DVM_VFS_CANCEL_CLOSE;
        return FVM_OK;
    }
    uint64_t id;
    DVM_Provider *selected = selectnode(root, path, &id);
    if (!selected)
    {
        return FVM_NOT_FOUND;
    }
    OpenNode *node = calloc(1, sizeof(*node));
    FVM_Result result = node ? dvm_provider_open(selected, ".", mode, &node->inner) : FVM_OUT_OF_MEMORY;
    dvm_provider_release(selected);
    if (result != FVM_OK)
    {
        free(node);
        return result;
    }
    node->identity = id;
    const char *slash = strrchr(path, '/');
    strcpy(node->name, slash ? slash+1 : path);
    node->ops.info = nodeinfo;
    node->ops.read_chunk = node->inner.ops->read || node->inner.ops->read_chunk ? readnode : NULL;
    node->ops.write_chunk = node->inner.ops->write || node->inner.ops->write_chunk ? writenode : NULL;
    node->ops.seek = node->inner.ops->seek ? seeknode : NULL;
    node->ops.close = closenode;
    out->ops = &node->ops;
    out->state = node;
    out->native_flags = node->inner.native_flags;
    out->close_ready = closeready;
    return FVM_OK;
}


/* EN/RU: Synthetic directories permit read/search to this granted root, never writes.
 * Synthetic directories разрешают read/search выданному root, не записи. */
static FVM_Result accessroot(void *state, const char *path, unsigned access)
{
    (void)state;
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    if ((!strcmp(path, ".") || !strcmp(path, "movie")) && !(access & ~(FVM_ACCESS_READ | FVM_ACCESS_EXEC)))
    {
        return FVM_OK;
    }
    return FVM_UNSUPPORTED;
}


/* EN/RU: Forward only invalidation into an independent journal pin; no provider or transport work.
 * Передать invalidation в independent journal pin без provider/transport work. */
static void changed(void *state, const DVM_ChangeEvent *event)
{
    changespost(state, event->kinds, 0, 0);
}


/* EN/RU: Consume the callback's independent journal pin after all admitted deliveries retire.
 * Поглотить independent journal pin после admitted deliveries. */
static void releasejournal(void *state)
{
    dvm_changes_destroy(state);
}


/* EN/RU: Consume native state after file/cursor users retire; callbacks retain only their journal.
 * Поглотить native state после file/cursor users; callbacks удерживают только свой journal. */
static void destroy(void *state)
{
    OfferRoot *root = state;
    dvm_notice_unsubscribe(root->tap);
    dvm_changes_destroy(root->journal);
    dvm_movie_offer_release(root->source);
    dvm_provider_release(root->control);
    free(root);
}


/* EN/RU: See offer.h; no root is published without all authority/lifetime pins and native notice setup.
 * См. offer.h; root выдаётся после authority/lifetime pins и native notice setup. */
DVM_Provider *styxmovieofferroot(DVM_MovieOffer *source, DVM_Provider *control)
{
    static const DVM_ProviderOps operations = {info, openroot, destroy, DVM_PROVIDER_STABLE_BIND, accessroot};
    FVM_FileInfo metadata;
    if (!source || !control || !dvm_provider_bindable_at(control, ".") ||
        !(dvm_provider_native_flags(control) & DVM_PROVIDER_CANCEL_PATH) ||
        dvm_provider_info(control, ".", &metadata) != FVM_OK || metadata.directory)
    {
        return NULL;
    }
    OfferRoot *state = calloc(1, sizeof(*state));
    if (!state)
    {
        return NULL;
    }
    if (!dvm_movie_offer_retain(source))
    {
        free(state);
        return NULL;
    }
    state->source = source;
    if (!dvm_provider_retain(control))
    {
        destroy(state);
        return NULL;
    }
    state->control = control;
    state->journal = dvm_movie_offer_observe(source);
    DVM_Provider *root = state->journal ? dvm_provider_create_native(&operations, state, NULL, 1, DVM_PROVIDER_CANCEL_PATH) : NULL;
    if (!root)
    {
        destroy(state);
        return NULL;
    }
    DVM_Changes *journal = dvm_provider_changes(root);
    if (changesretain(journal))
    {
        state->tap = dvm_notice_subscribe(dvm_changes_notices(state->journal), changed, journal, releasejournal);
        if (!state->tap)
        {
            dvm_changes_destroy(journal);
        }
    }
    if (!state->tap)
    {
        dvm_provider_release(root);
        return NULL;
    }
    return root;
}
