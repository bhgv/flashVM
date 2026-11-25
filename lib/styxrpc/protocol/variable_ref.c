/* Copyright (c) 2026 bhgv
 * EN: Encode a bounded variable reference without pointers or path text.
 * RU: Кодирование ограниченной ссылки без указателей и строк пути.
 */
#include "styxrpc/variable_ref.h"


/* EN/RU: See variable_ref.h. / См. variable_ref.h. */
int styxrpc_int_ref_encode(const StyxRpcIntRef *ref, void *bytes, size_t capacity)
{
    if (!ref || !bytes || capacity < STYXRPC_INT_REF_SIZE ||
        ref->pid < STYXRPC_INT_REF_FIRST_PID || (ref->writable != 0 && ref->writable != 1))
    {
        return 0;
    }
    unsigned char *wire = bytes;
    wire[0] = 'V';
    wire[1] = 'R';
    wire[2] = 1;
    wire[3] = (unsigned char)ref->writable;
    for (unsigned i = 0; i < 4; ++i)
    {
        wire[4 + i] = (unsigned char)(ref->pid >> (24 - 8 * i));
    }
    return 1;
}


/* EN/RU: See variable_ref.h. / См. variable_ref.h. */
int styxrpc_int_ref_decode(const void *bytes, size_t size, StyxRpcIntRef *out)
{
    const unsigned char *wire = bytes;
    if (!wire || !out || size != STYXRPC_INT_REF_SIZE || wire[0] != 'V' || wire[1] != 'R' ||
        wire[2] != 1 || wire[3] > 1)
    {
        return 0;
    }
    uint32_t pid = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        pid = (pid << 8) | wire[4 + i];
    }
    if (pid < STYXRPC_INT_REF_FIRST_PID)
    {
        return 0;
    }
    *out = (StyxRpcIntRef){pid, wire[3]};
    return 1;
}
