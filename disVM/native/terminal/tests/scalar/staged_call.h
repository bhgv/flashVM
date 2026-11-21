/* Copyright (c) 2026 bhgv
 * EN: Exercise delayed dispatch on the existing mounted function fid.
 * RU: Проверить отложенный вызов на открытом fid функции.
 */
#ifndef STYX_SCALAR_STAGED_CALL_TEST_H
#define STYX_SCALAR_STAGED_CALL_TEST_H
#include "call_scope.h"
#include "flashvm/srecord.h"

#ifndef STYX_SCALAR_GUEST

/* EN: Write a staged request and return its next byte offset, or zero on refusal.
 * RU: Записать отложенный запрос и вернуть следующее смещение либо ноль при отказе. */
static size_t stage_request(StyxFid *fid, const void *request, size_t size)
{
    unsigned char bytes[128];
    size_t used;
    StyxTransfer transfer;
    if (fvm_srecord_write(STYX_CALL_STAGE_TAG, request, size, bytes, sizeof(bytes), &used) != FVM_S_OK)
    {
        return 0;
    }
    styxclientwrite(fid, 0, bytes, used, &transfer);
    return transfer.success && transfer.confirmed && transfer.count == used ? used : 0;
}


/* EN: Clunk a staged sibling fid before committing the original request.
 * RU: Закрыть соседний fid с отложенным запросом перед завершением исходного. */
static int staged_call(StyxClient *client, StyxFid *fid, const void *request, size_t size)
{
    size_t offset = stage_request(fid, request, size);
    StyxFid *abandoned = styxclientwalk(client, "mounted/compute.dis/compute");
    int isolated = abandoned && styxclientopen(abandoned, ORDWR) && stage_request(abandoned, request, size);
    if (abandoned)
    {
        styxclientclunk(abandoned);
    }
    if (!isolated || !offset)
    {
        return 0;
    }
    unsigned char bytes[128];
    size_t used;
    StyxTransfer transfer;
    styxclientread(fid, 0, bytes, sizeof(bytes), &transfer);
    if (!transfer.success || transfer.count ||
        fvm_srecord_write(STYX_CALL_END_TAG, NULL, 0, bytes, sizeof(bytes), &used) != FVM_S_OK)
    {
        return 0;
    }
    styxclientwrite(fid, offset, bytes, used, &transfer);
    return transfer.success && transfer.confirmed && transfer.count == used;
}
#endif
#endif
