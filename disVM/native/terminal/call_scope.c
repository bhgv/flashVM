/* Copyright (c) 2026 bhgv
 * EN: Encode a session-local PID with one already bounded call frame.
 * RU: Кодировать PID сессии вместе с одним ограниченным кадром вызова.
 */
#include "call_scope.h"
#include "tuple_record.h"
#include "flashvm/srecord.h"


/* EN/RU: See call_scope.h. / См. call_scope.h. */
int styxcall_scope_write(uint32_t pid, const void *call, size_t size, void *bytes, size_t capacity, size_t *used)
{
    unsigned char identity[] = {pid >> 24, pid >> 16, pid >> 8, pid};
    return pid && call && size && size <= STYX_FUNCTION_FRAME_CAPACITY &&
        fvm_srecord_write_pair(STYX_SCOPED_CALL_TAG, identity, sizeof(identity), call, size,
            bytes, capacity, used) == FVM_S_OK;
}


/* EN/RU: See call_scope.h. / См. call_scope.h. */
int styxcall_scope_read(const void *bytes, size_t size, uint32_t *pid, const void **call, size_t *length)
{
    FVM_SRecord record;
    size_t used;
    if (!pid || !call || !length || size > STYX_FUNCTION_FRAME_CAPACITY + STYX_SCOPED_CALL_OVERHEAD ||
        fvm_srecord_read(bytes, size, STYX_FUNCTION_FRAME_CAPACITY + STYX_SCOPED_CALL_OVERHEAD,
            &record, &used) != FVM_S_OK || size != used || record.tag != STYX_SCOPED_CALL_TAG ||
        record.size <= 4 || record.size - 4 > STYX_FUNCTION_FRAME_CAPACITY)
    {
        return 0;
    }
    const unsigned char *p = record.payload;
    uint32_t identity = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
    if (!identity)
    {
        return 0;
    }
    *pid = identity;
    *call = p + 4;
    *length = record.size - 4;
    return 1;
}
