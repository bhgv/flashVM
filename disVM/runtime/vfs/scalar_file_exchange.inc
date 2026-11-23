/* Copyright (c) 2026 bhgv
 * EN: Exchange request and reply bytes over a captured Styx function, with a scoped caller lease.
 * RU: Обмен байтами через захваченный файл функции Styx с закреплённым контекстом клиента.
 */


/* EN: Write an exact frame and require a confirmed full acknowledgement.
 * RU: Записать точный кадр и потребовать подтверждения всех байтов. */
static int write_frame(DVM_VfsHandle *handle, const void *bytes, size_t size)
{
    DVM_IoResult sent = dvm_vfs_write_chunk(handle, bytes, size);
    return sent.status == FVM_OK && (sent.flags & DVM_IO_PREFIX) &&
        !(sent.flags & DVM_IO_UNCERTAIN) && sent.count == size;
}


/* EN: Stage and commit on one fid when explicitly advertised by the owner.
 * RU: Сохранить и завершить запрос на одном fid, если владелец объявил этот режим. */
static int submit_frame(DVM_VfsHandle *handle, const void *bytes, size_t size, int explicit_end)
{
    if (explicit_end == STYX_CALL_RECORD_FRAMING)
    {
        const unsigned char *cursor = bytes;
        while (size)
        {
            DVM_IoResult sent = dvm_vfs_write_chunk(handle, cursor, size);
            if (sent.status != FVM_OK || !(sent.flags & DVM_IO_PREFIX) ||
                (sent.flags & DVM_IO_UNCERTAIN) || !sent.count || sent.count > size)
            {
                return 0;
            }
            cursor += sent.count;
            size -= sent.count;
        }
        return 1;
    }
    if (!explicit_end)
    {
        return write_frame(handle, bytes, size);
    }
    unsigned char staged[STYX_FUNCTION_FRAME_CAPACITY + 2 * STYX_SCOPED_CALL_OVERHEAD];
    unsigned char end[STYX_SCOPED_CALL_OVERHEAD];
    unsigned char context[32];
    size_t count, end_size, context_size;
    int context_ready = explicit_end != STYX_CALL_PATH_FRAMING ||
        fvm_srecord_write(STYX_CALL_CONTEXT_TAG, STYX_CALL_CONTEXT_SELF, sizeof(STYX_CALL_CONTEXT_SELF) - 1,
            context, sizeof(context), &context_size) == FVM_S_OK;
    if (!context_ready || (explicit_end == STYX_CALL_PATH_FRAMING &&
        !write_frame(handle, context, context_size)) || !size)
    {
        return 0;
    }
    const unsigned char *cursor = bytes;
    for (size_t offset = 0; offset < size; )
    {
        size_t chunk = size - offset;
        if (chunk > STYX_FUNCTION_FRAME_CAPACITY)
        {
            chunk = STYX_FUNCTION_FRAME_CAPACITY;
        }
        unsigned long tag = offset ? STYX_CALL_CHUNK_TAG : STYX_CALL_STAGE_TAG;
        if (fvm_srecord_write(tag, cursor + offset, chunk, staged, sizeof(staged), &count) != FVM_S_OK ||
            !write_frame(handle, staged, count))
        {
            return 0;
        }
        offset += chunk;
    }
    return fvm_srecord_write(STYX_CALL_END_TAG, NULL, 0, end, sizeof(end), &end_size) == FVM_S_OK &&
        write_frame(handle, end, end_size);
}


/* EN: Borrow a native mailbox, local channel roots and the current wait token.
 * RU: Заимствовать нативный ящик, корни каналов и текущий токен ожидания. */
typedef struct ExchangeChannels
{
    DVM_ChannelBus *bus; /* Native provider state, alive for the exchange. */
    DVM_CallChannels *channels; /* Dis objects used only by this caller. */
    int *native; /* Restored on return even after a failed match. */
    DVM_ReturnedLease *result; /* Optional original function fid owner after success. */
} ExchangeChannels;


/* EN: Exchange one function frame, servicing optional reverse UniBus requests.
 * RU: Обменять кадр функции, обслуживая необязательные встречные запросы UniBus. */
static int exchange_buffers(DVM_ScalarFiles *files, const void *request, size_t size,
    unsigned char *reply, size_t capacity, size_t *received, const ExchangeChannels *side)
{
    DVM_VfsHandle handle = {0};
    if (dvm_provider_open(files->request, ".", "rw", &handle) != FVM_OK)
    {
        return -1;
    }
    int result = -1;
    if ((handle.native_flags & (DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_SEEK)) ==
        (DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_SEEK))
    {
        result = 0;
        long position;
        if (submit_frame(&handle, request, size, files->explicit_end) &&
            dvm_vfs_seek(&handle, 0, FVM_FILE_SEEK_START, &position) == FVM_OK)
        {
            size_t used = 0;
            unsigned pending = 0;
            const struct timespec pause = {0, 10000000};
            while (used < capacity)
            {
                DVM_IoResult got = dvm_vfs_read_chunk(&handle, reply + used, capacity - used);
                if (got.status != FVM_OK || !(got.flags & DVM_IO_PREFIX) ||
                    (got.flags & DVM_IO_UNCERTAIN) || got.count > capacity - used)
                {
                    break;
                }
                if (!used && !got.count && (files->explicit_end == STYX_CALL_PATH_FRAMING ||
                    files->explicit_end == STYX_CALL_RECORD_FRAMING))
                {
                    if (dvm_cancel_requested(dvm_cancel_current()) ||
                        (side && side->bus && !dvm_call_channel_progress(side->bus, side->channels, side->native)) ||
                        (files->explicit_end != STYX_CALL_RECORD_FRAMING && ++pending == 500))
                    {
                        break;
                    }
                    nanosleep(&pause, NULL);
                    continue;
                }
                used += got.count;
                if (!got.count || got.stop == DVM_IO_EOF)
                {
                    *received = used;
                    result = 1;
                    break;
                }
            }
        }
    }
    if (result == 1 && side && side->result && !dvm_returned_lease_take(side->result, &handle))
    {
        result = 0;
    }
    dvm_vfs_close(&handle);
    return result;
}


/* EN: Reuse a single buffer for request and reply after the request is sent.
 * RU: Использовать один буфер после полной отправки запроса. */
static int exchange_frame(DVM_ScalarFiles *files, unsigned char *bytes, size_t size, size_t capacity,
    size_t *received)
{
    return exchange_buffers(files, bytes, size, bytes, capacity, received, NULL);
}


/* EN: Wrap a compiled call and publish the current process through its reverse lease.
 * RU: Упаковать вызов по схеме и опубликовать текущий процесс во встречной сессии. */
static int exchange_compiled(DVM_ScalarFiles *files, DVM_Namespace *group, DVM_LaunchFiles *descriptors,
    uint32_t pid, unsigned char *bytes, size_t size, size_t capacity, size_t *received)
{
    if (!files->caller)
    {
        return exchange_frame(files, bytes, size, capacity, received);
    }
    size_t limit = capacity > STYX_FUNCTION_FRAME_CAPACITY + 1 ?
        STYX_MANAGED_LIST_LIMIT : STYX_FUNCTION_FRAME_CAPACITY;
    size_t scoped_capacity = limit + STYX_SCOPED_LARGE_OVERHEAD;
    size_t used;
    DVM_CallerBinding binding;
    if (!files->dynamic || !dvm_provider_caller_binding(files->request, &binding))
    {
        return -1;
    }
    unsigned char *scoped = malloc(scoped_capacity);
    if (!scoped || !styxcall_scope_write_bounded(pid, bytes, size, limit, scoped, scoped_capacity, &used))
    {
        free(scoped);
        return -1;
    }
    DVM_Provider *tree = dvm_program_current(group, descriptors, pid);
    void *lease = tree ? binding.begin(binding.state, files->request, tree, pid) : NULL;
    int result = lease ? exchange_frame(files, scoped, used, scoped_capacity, received) : -1;
    if (lease)
    {
        binding.end(binding.state, lease);
    }
    dvm_provider_release(tree);
    if (result == 1)
    {
        if (*received >= capacity)
        {
            result = 0;
        }
        else
        {
            memcpy(bytes, scoped, *received);
        }
    }
    free(scoped);
    return result;
}


/* EN/RU: Exchange one schema-checked managed frame through the existing opened-function transport.
 * Обменять проверенный managed-кадр через существующий транспорт файла функции.
 */
int dvm_managed_function_exchange(DVM_ScalarFiles *files, DVM_Namespace *group,
    DVM_LaunchFiles *descriptors, uint32_t pid, const FVM_SRecord *arguments,
    const void *request, size_t size, unsigned char *reply, size_t capacity, size_t *received,
    DVM_CallChannels *channels, int *native)
{
    if (!files || !files->dynamic || !arguments || !request || !size || !reply || !capacity || !received)
    {
        return -1;
    }
    if (!files->caller)
    {
        return channels->count ? -1 : exchange_buffers(files, request, size, reply, capacity, received, NULL);
    }
    unsigned char first[STYX_SCALAR_RECORD_CAPACITY];
    size_t prefix, used;
    DVM_CallerBinding binding;
    if (!pid || !dvm_provider_caller_binding(files->request, &binding) ||
        !styxscalarwrite(STYX_CALL_BIG_TAG, STYX_SCALAR_VALUE, pid, first, sizeof(first), &prefix) ||
        !styxmanaged_request_tag(files->managed_tag) ||
        fvm_srecord_write_pair(files->managed_tag, first, prefix, arguments->payload, arguments->size,
            reply, capacity, &used) != FVM_S_OK || used > STYX_MANAGED_LIST_LIMIT)
    {
        return -1;
    }
    ExchangeChannels side = {.channels = channels, .native = native,
        .result = channels->imports ? channels->imports->lease : NULL};
    DVM_Provider *bus = channels->count ? dvm_channel_bus_create(channels->schemas, channels->count, &side.bus) : NULL;
    if (channels->count && !bus)
    {
        return -1;
    }
    DVM_Provider *tree = dvm_program_current_unibus(group, descriptors, pid, bus);
    void *lease = tree ? binding.begin(binding.state, files->request, tree, pid) : NULL;
    DVM_ScalarFiles route = {.request = files->request, .explicit_end = STYX_CALL_RECORD_FRAMING};
    int result = lease ? exchange_buffers(&route, reply, used, reply, capacity, received,
        bus || side.result ? &side : NULL) : -1;
    dvm_channel_bus_seal(side.bus);
    if (lease)
    {
        binding.end(binding.state, lease);
    }
    dvm_provider_release(tree);
    dvm_provider_release(bus);
    return result;
}
