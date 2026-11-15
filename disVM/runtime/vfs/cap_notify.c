/* Copyright (c) 2026 bhgv
 * EN: Ordinary publication notification node feeding existing automatic VFS watches.
 * RU: Ordinary publication notification node для automatic VFS watches.
 */
#include "cap_internal.h"


/* EN/RU: A publication node is readonly, inert data with a live source journal.
 * Publication node — readonly inert data с live source journal. */
static FVM_Result info(void *state, FVM_FileInfo *output)
{
    (void)state;
    memset(output, 0, sizeof(*output));
    strcpy(output->name, "publication");
    output->mode = 0444;
    return FVM_OK;
}


/* EN/RU: Notification data is empty; the authoritative requests live in next/reply.
 * Data пусты; authoritative requests находятся в next/reply, не в byte stream. */
static FVM_Result read_empty(void *state, void *bytes, size_t size, size_t *received)
{
    (void)state;
    (void)bytes;
    (void)size;
    *received = 0;
    return FVM_OK;
}


/* EN/RU: Consume only the private cursor placeholder.
 * Освободить private cursor placeholder. */
static void close_cursor(void *state)
{
    free(state);
}
static const DVM_VfsOps ops = {info, read_empty, NULL, NULL, NULL, close_cursor}; /* Immutable notification cursor. */


/* EN/RU: Expose exactly the notification node, never descendants or write access.
 * Открыть только notification node, без descendants/write access. */
static FVM_Result open_node(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    (void)state;
    if (strcmp(path, ".") || strcmp(mode, "r"))
    {
        return FVM_UNSUPPORTED;
    }
    output->state = calloc(1, 1);
    output->ops = output->state ? &ops : NULL;
    return output->state ? FVM_OK : FVM_OUT_OF_MEMORY;
}


/* EN/RU: Copy root-only metadata, no external provider work.
 * Скопировать root-only metadata, без внешнего provider work. */
static FVM_Result root_info(void *state, const char *path, FVM_FileInfo *output)
{
    return strcmp(path, ".") ? FVM_NOT_FOUND : info(state, output);
}


/* EN/RU: Exact-node readonly access, independent of requested resource authority.
 * Exact-node readonly access, независимо от прав запрошенного ресурса. */
static FVM_Result access_node(void *state, const char *path, unsigned access)
{
    (void)state;
    return !strcmp(path, ".") && !(access & ~FVM_ACCESS_READ) ? FVM_OK : FVM_ERROR;
}
static const DVM_ProviderOps factory = {root_info, open_node, close_cursor, DVM_PROVIDER_STABLE_BIND, access_node}; /* Journal-only ordinary node. */


/* EN/RU: See internal header; factory owns a private scalar placeholder.
 * См. internal header; factory владеет private scalar placeholder. */
DVM_Provider *dvm_cap_events(void)
{
    void *state = calloc(1, 1);
    if (!state)
    {
        return NULL;
    }
    DVM_Provider *provider = dvm_provider_create(&factory, state);
    if (!provider)
    {
        free(state);
    }
    return provider;
}


/* EN/RU: Called under service serialization, outside publication mutex; native push only.
 * Под service serialization, вне publication mutex; только native push. */
void dvm_cap_notify(void *context)
{
    DVM_CapService *service = context;
    DVM_ChangeCursor cursor = {0};
    DVM_ChangeBatch batch;
    DVM_Changes *journal = dvm_provider_changes(service->events);
    if (dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) == 1)
    {
        if (batch.event.generation == UINT64_MAX)
        {
            dvm_changes_reconnect(journal, batch.event.session);
        }
        else
        {
            DVM_ChangeEvent event = {batch.event.session, batch.event.generation + 1, DVM_CHANGE_CONTENT, 0, 0};
            dvm_changes_notify(journal, &event);
        }
    }
}
