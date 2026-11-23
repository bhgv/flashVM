/* Copyright (c) 2026 bhgv
 * EN: Read bounded diagnostic metadata from the native channel registry.
 * RU: Чтение ограниченных метаданных диагностики из нативного реестра.
 */


/* EN/RU: See channel_export.h. / См. channel_export.h. */
void styxchannel_export_annotate(DVM_Provider *provider, uint32_t fid, uint32_t pid)
{
    ChannelExport *control = state_of(provider);
    if (control)
    {
        if (fid != UINT32_MAX)
        {
            __atomic_store_n(&control->fid, fid, __ATOMIC_RELAXED);
        }
        if (pid)
        {
            __atomic_store_n(&control->pid, pid, __ATOMIC_RELAXED);
        }
    }
}


/* EN/RU: See channel_export.h. / См. channel_export.h. */
size_t styxchannel_export_snapshot(StyxChannelExports *exports, char *text, size_t capacity)
{
    size_t used = 0;
    fvm_port_sync_lock(exports->lock);
    for (ChannelExport *entry = exports->head; entry && used < capacity; entry = entry->next)
    {
        fvm_port_sync_lock(entry->lock);
        uint32_t fid = __atomic_load_n(&entry->fid, __ATOMIC_RELAXED);
        uint32_t pid = __atomic_load_n(&entry->pid, __ATOMIC_RELAXED);
        int length;
        if (fid == UINT32_MAX)
        {
            length = snprintf(text + used, capacity - used, "derived pid=%u state=%s\n",
                pid, entry->bus ? "published" : "pending");
        }
        else
        {
            length = snprintf(text + used, capacity - used, "fid=%u pid=%u state=%s\n",
                fid, pid, entry->bus ? "published" : "pending");
        }
        fvm_port_sync_unlock(entry->lock);
        if (length < 0 || (size_t)length >= capacity - used)
        {
            break;
        }
        used += (size_t)length;
    }
    fvm_port_sync_unlock(exports->lock);
    return used;
}
