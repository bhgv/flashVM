/* Copyright (c) 2026 bhgv
 * EN: Per-fid managed call validation and staged request transport.
 * RU: Проверка управляемого вызова и сборка запроса на одном fid.
 */


/* EN: Accept legacy requests or a staged request followed by an explicit end record.
 * RU: Принять прежний запрос либо отложенный запрос с отдельной записью конца. */
char *styxscalarfunctionwrite(void *state, const char *bytes, ulong count, vlong offset)
{
    FunctionFid *fid = state;
    if (!fid || fid->running || fid->submitted || offset < 0 || (uint64_t)offset != fid->write_offset)
    {
        return Ebadarg;
    }
    if (fid->function.managed_method && fid->function.program_root)
    {
        if (!count || count > STYX_MANAGED_LIST_LIMIT - fid->staged_size)
        {
            return Ebadarg;
        }
        size_t size = fid->staged_size + count;
        memcpy(fid->staged + fid->staged_size, bytes, count);
        FVM_SRecord request;
        size_t used;
        FVM_SResult parsed = fvm_srecord_read(fid->staged, size, STYX_MANAGED_LIST_LIMIT, &request, &used);
        if (parsed == FVM_S_MORE && size < STYX_MANAGED_LIST_LIMIT)
        {
            fid->staged_size = size;
            fid->write_offset += count;
            return nil;
        }
        if (parsed != FVM_S_OK || used != size || request.tag != STYX_MANAGED_REQUEST_TAG)
        {
            return Ebadarg;
        }
        size_t previous = fid->staged_size;
        fid->staged_size = size;
        char *failure = fid->workers ? schedule(fid) : dispatch(fid, fid->staged, size);
        if (failure && !fid->submitted)
        {
            fid->staged_size = previous;
        }
        else
        {
            fid->write_offset += count;
        }
        return failure;
    }
    FVM_SRecord record;
    size_t used;
    int complete = count <= fid->stage_capacity + STYX_SCOPED_CALL_OVERHEAD &&
        fvm_srecord_read(bytes, count, fid->stage_capacity + STYX_SCOPED_CALL_OVERHEAD,
            &record, &used) == FVM_S_OK && used == count;
    if (fid->staged_size)
    {
        if (complete && record.tag == STYX_CALL_CHUNK_TAG)
        {
            if (!record.size || record.size > fid->stage_capacity - fid->staged_size)
            {
                return Ebadarg;
            }
            memcpy(fid->staged + fid->staged_size, record.payload, record.size);
            fid->staged_size += record.size;
            fid->write_offset += count;
            return nil;
        }
        if (!complete || record.tag != STYX_CALL_END_TAG || record.size)
        {
            return Ebadarg;
        }
        if (fid->workers)
        {
            return fid->context_written ? schedule(fid) : Ebadarg;
        }
        return dispatch(fid, fid->staged, fid->staged_size);
    }
    if (complete && record.tag == STYX_CALL_CONTEXT_TAG && !offset)
    {
        if (!styxcall_context(record.payload, record.size, &fid->context_pid))
        {
            return Ebadarg;
        }
        fid->context_written = 1;
        fid->write_offset = count;
        return nil;
    }
    if (complete && record.tag == STYX_CALL_STAGE_TAG)
    {
        if (!record.size || record.size > fid->stage_capacity)
        {
            return Ebadarg;
        }
        memcpy(fid->staged, record.payload, record.size);
        fid->staged_size = record.size;
        fid->write_offset += count;
        return nil;
    }
    return !fid->workers && !fid->context_written && count <= fid->stage_capacity ? dispatch(fid, bytes, count) : Ebadarg;
}


/* EN/RU: Copy a reply slice to bytes and report its count; reject reads before a request.
 * Копировать часть ответа в bytes и вернуть размер; отказать до получения запроса. */
char *styxscalarfunctionread(void *state, char *bytes, ulong *count, vlong offset)
{
    FunctionFid *fid = state;
    if (fid && fid->running)
    {
        if (!__atomic_load_n(&fid->ready, __ATOMIC_ACQUIRE))
        {
            *count = 0;
            return offset == 0 ? nil : Ebadarg;
        }
        if (fid->failure)
        {
            return fid->failure;
        }
    }
    if (fid && fid->staged_size && !fid->submitted && offset == 0)
    {
        *count = 0;
        return nil;
    }
    if (!fid || !fid->size || offset < 0)
    {
        return Ebadarg;
    }
    if ((uvlong)offset >= fid->size)
    {
        *count = 0;
        return nil;
    }
    size_t available = fid->size - (size_t)offset;
    if (*count > available)
    {
        *count = available;
    }
    memcpy(bytes, fid->reply + offset, *count);
    return nil;
}
