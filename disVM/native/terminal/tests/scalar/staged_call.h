/* Copyright (c) 2026 bhgv
 * EN: Exercise delayed dispatch on the existing mounted function fid.
 * RU: Проверить отложенный вызов на открытом fid функции.
 */
#ifndef STYX_SCALAR_STAGED_CALL_TEST_H
#define STYX_SCALAR_STAGED_CALL_TEST_H
#include "call_scope.h"
#include "flashvm/srecord.h"

#ifndef STYX_SCALAR_GUEST

/* EN: Stage a request, poll before dispatch, then dispatch on the same fid.
 * RU: Записать запрос, проверить чтение до выполнения, выполнить на том же fid. */
static int staged_call(StyxFid *fid, const void *request, size_t size)
{
    unsigned char bytes[128];
    size_t used;
    StyxTransfer transfer;
    if (fvm_srecord_write(STYX_CALL_STAGE_TAG, request, size, bytes, sizeof(bytes), &used) != FVM_S_OK)
    {
        return 0;
    }
    styxclientwrite(fid, 0, bytes, used, &transfer);
    if (!transfer.success || transfer.count != used)
    {
        return 0;
    }
    size_t offset = used;
    styxclientread(fid, 0, bytes, sizeof(bytes), &transfer);
    if (!transfer.success || transfer.count || fvm_srecord_write(STYX_CALL_END_TAG, NULL, 0, bytes, sizeof(bytes), &used) != FVM_S_OK)
    {
        return 0;
    }
    styxclientwrite(fid, offset, bytes, used, &transfer);
    return transfer.success && transfer.confirmed && transfer.count == used;
}
#endif
#endif
