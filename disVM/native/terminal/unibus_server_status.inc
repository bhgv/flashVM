/* Copyright (c) 2026 bhgv
 * EN: Mounted UniBus capability marker and on-demand native diagnostics.
 * RU: Профиль смонтированной UniBus и нативная диагностика по запросу.
 */


/* EN: Resolve a module-level metadata file within the reserved qid range.
 * RU: Найти файл метаданных модуля в зарезервированном диапазоне qid. */
static StyxChannelExports *channel_metadata(Path path, Path first)
{
    return path >= first && path - first < current->count ?
        current->modules[path - first].channel_exports : NULL;
}


/* EN: Hold one stable snapshot across small reads on the same fid.
 * RU: Удерживать один снимок между частичными чтениями одного fid. */
typedef struct ChannelStatus
{
    size_t size; /* Number of valid text bytes in snapshot. */
    char text[STYX_CHANNEL_STATUS_CAPACITY]; /* Contains metadata only, never tokens. */
} ChannelStatus;


/* EN: Allocate and attach a native snapshot to this diagnostic fid.
 * RU: Выделить и прикрепить нативный снимок к диагностическому fid. */
static char *open_channel_status(StyxChannelExports *exports, int mode)
{
    if (mode != OREAD)
    {
        return Eperm;
    }
    ChannelStatus *status = calloc(1, sizeof(*status));
    if (!status)
    {
        return Enomem;
    }
    unsigned occupied = 0, capacity = 0;
    styxmounttagcounts(current->caller_root, &occupied, &capacity);
    int workers = __atomic_load_n(&current->function_workers, __ATOMIC_ACQUIRE);
    int length = snprintf(status->text, sizeof(status->text),
        "reverse_styx_tags=%u/%u function_workers=%d\n", occupied, capacity, workers);
    if (length < 0 || (size_t)length >= sizeof(status->text) ||
        styxsetauxowned(&current->server, status, free) < 0)
    {
        free(status);
        return Eperm;
    }
    status->size = (size_t)length;
    status->size += styxchannel_export_snapshot(exports, status->text + status->size,
        sizeof(status->text) - status->size);
    return nil;
}


/* EN: Copy one acknowledged positional read from constant data or the opened snapshot.
 * RU: Скопировать позиционное чтение из постоянных данных или открытого снимка. */
static char *read_channel_metadata(Path path, char *bytes, ulong *count, vlong offset)
{
    if (offset < 0)
    {
        return Ebadarg;
    }
    const char *source = STYX_CHANNEL_PROFILE;
    size_t size = sizeof(STYX_CHANNEL_PROFILE) - 1;
    if (channel_metadata(path, FIRST_CHANNEL_STATUS))
    {
        const ChannelStatus *status = styxgetaux(&current->server);
        if (!status)
        {
            return Eperm;
        }
        source = status->text;
        size = status->size;
    }
    size_t available = (uvlong)offset < size ? size - (size_t)offset : 0;
    if (*count > available)
    {
        *count = available;
    }
    if (*count)
    {
        memcpy(bytes, source + offset, *count);
    }
    return nil;
}
