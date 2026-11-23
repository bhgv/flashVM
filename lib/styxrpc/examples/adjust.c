/* Copyright (c) 2026 bhgv
 * EN: A string and an int array flow in and a ref Cell is written back.
 * RU: Строка и массив int приходят на вход, а ref Cell обновляется при возврате.
 */
#include "adjust.h"
#include "call_record.h"
#include "scalar_record.h"
#include <string.h>

typedef struct AdjustBuffers
{
    unsigned char result[1024]; /* One ordered result tuple. */
    unsigned char update[32];   /* One rewritten Cell when its counter changes. */
} AdjustBuffers;

static AdjustBuffers staging; /* Borrowed across begin return on this serial dispatcher. */


/* EN: Borrow the first complete child and advance cursor by its encoded byte extent.
 * RU: Заимствовать первого потомка и продвинуть cursor на длину его записи. */
static int next(FVM_SRecord *cursor, FVM_SRecord *node, const unsigned char **bytes, size_t *size)
{
    *bytes = cursor->payload;
    if (fvm_srecord_read(*bytes, cursor->size, STYXRPC_MANAGED_LIMIT, node, size) != FVM_S_OK)
    {
        return 0;
    }
    cursor->payload += *size;
    cursor->size -= *size;
    return 1;
}


/* EN: Add the array's first element to Cell.counter and return the supplied string and new int.
 * RU: Прибавить первый элемент к Cell.counter и вернуть входную строку и новый int. */
static StyxRpcStatus adjust(void *context, const FVM_SRecord *arguments, StyxRpcOutput *output)
{
    AdjustBuffers *buffers = context;
    FVM_SRecord values = *arguments, text, array, ref;
    const unsigned char *text_bytes, *ignored;
    size_t text_size, length;
    if (!next(&values, &text, &text_bytes, &text_size) ||
        !next(&values, &array, &ignored, &length) || array.tag != STYX_MANAGED_ARRAY_TAG ||
        !next(&values, &ref, &ignored, &length) || ref.tag != STYX_MANAGED_REF_TAG || ref.size < 2)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    FVM_SRecord element;
    const unsigned char *first;
    int64_t increment, counter;
    StyxScalarKind kind;
    if (!next(&array, &element, &first, &length) ||
        !styxscalarread(STYX_CALL_INT_TAG, first, length, &kind, &increment) ||
        !styxscalarread(STYX_CALL_INT_TAG, ref.payload + 1, ref.size - 1, &kind, &counter) ||
        counter + increment < INT32_MIN || counter + increment > INT32_MAX)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    unsigned char scalar[16];
    size_t size;
    const unsigned char index = 0;
    if (!styxscalarwrite(STYX_CALL_INT_TAG, STYX_SCALAR_VALUE, counter + increment, scalar, sizeof(scalar), &size) ||
        fvm_srecord_write_pair(STYX_CALL_TUPLE_TAG, text_bytes, text_size, scalar, size,
            buffers->result, sizeof(buffers->result), &output->result_size) != FVM_S_OK ||
        (increment && fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &index, 1, scalar, size,
            buffers->update, sizeof(buffers->update), &output->update_size) != FVM_S_OK))
    {
        return STYXRPC_REMOTE_ERROR;
    }
    output->result = buffers->result;
    output->updates = buffers->update;
    output->changed = increment ? 1 : 0;
    return STYXRPC_OK;
}

const StyxRpcManagedExport adjust_export = {
    "adjust", UINT32_C(0xe6d83cc8), {"(sa[i]r[i])", "((si))"}, adjust, NULL, &staging
};
