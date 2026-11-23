/* Copyright (c) 2026 bhgv
 * EN: Check frame fragmentation, callback lifetime, cancellation and native reply validation.
 * RU: Проверка дробления кадров, жизненного цикла колбэков, отмены и разбора ответа.
 */
#include "styxrpc/managed.h"
#include "call_record.h"
#include <float.h>
#include <stdio.h>
#include <string.h>

typedef struct State
{
    unsigned begins;  /* Exact number of validated calls delivered. */
    unsigned cancels; /* Exact number of pending calls closed. */
} State;


/* EN: Mark one checked request as waiting, borrowing its arguments.
 * RU: Отметить проверенный запрос ожидающим, заимствуя аргументы. */
static StyxRpcStatus begin(void *context, const FVM_SRecord *arguments, StyxRpcOutput *output)
{
    State *state = context;
    (void)output;
    state->begins++;
    return arguments->size ? STYXRPC_MORE : STYXRPC_REMOTE_ERROR;
}


/* EN: Count retirement of a pending request with valid borrowed bytes.
 * RU: Посчитать отмену запроса с допустимыми заимствованными данными. */
static void cancel(void *context, const FVM_SRecord *arguments)
{
    State *state = context;
    if (arguments->size)
    {
        state->cancels++;
    }
}


/* EN: Check exact binary64 bits, including signed zero and a quiet NaN payload.
 * RU: Проверить точные биты binary64, включая знак нуля и нагрузку тихого NaN. */
static int real_bits(void)
{
    static const uint64_t patterns[] = {
        UINT64_C(0x0000000000000000), UINT64_C(0x8000000000000000),
        UINT64_C(0x7ff0000000000000), UINT64_C(0x7ff8000000000042),
        UINT64_C(0xc029000000000000)
    };
    double one = 1.0;
    uint64_t representation;
    if (sizeof(double) != sizeof(representation) || FLT_RADIX != 2 ||
        DBL_MANT_DIG != 53 || DBL_MAX_EXP != 1024)
    {
        return 1;
    }
    memcpy(&representation, &one, sizeof(representation));
    if (representation != UINT64_C(0x3ff0000000000000))
    {
        return 1;
    }
    for (size_t i = 0; i < sizeof(patterns) / sizeof(*patterns); ++i)
    {
        double input, output;
        uint64_t actual;
        unsigned char bytes[32];
        size_t used;
        memcpy(&input, patterns + i, sizeof(input));
        if (styxrpc_value_real(input, bytes, sizeof(bytes), &used) != STYXRPC_OK ||
            styxrpc_value_get_real(bytes, used, &output) != STYXRPC_OK)
        {
            return 0;
        }
        memcpy(&actual, &output, sizeof(actual));
        if (actual != patterns[i] ||
            styxrpc_value_get_real(bytes, used - 1, &output) != STYXRPC_INVALID ||
            memcmp(&output, patterns + i, sizeof(output)))
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Use two separate fids with one descriptor, then validate the selected reply.
 * RU: Использовать два fid с общим описанием и проверить выбранный ответ. */
int main(void)
{
    if (!real_bits())
    {
        fputs("binary64 managed record lost bits or accepted a truncated frame\n", stderr);
        return 1;
    }
    State state = {0};
    const StyxRpcManagedExport exported = {
        "echo", UINT32_C(0x12345678), {"(s)", "(s)"}, begin, cancel, &state
    };
    unsigned char value[24], request[32], first[32], second[32], reply_one[48], reply_two[48], scratch[48];
    size_t value_size, length, received;
    StyxRpcManagedCall a = {0}, b = {0};
    if (fvm_srecord_write(STYX_MANAGED_STRING_TAG, "A", 1, value, sizeof(value), &value_size) != FVM_S_OK ||
        styxrpc_managed_request(exported.signature, &exported.schema,
            value, value_size, request, sizeof(request), &length) != STYXRPC_OK ||
        styxrpc_managed_open(&a, &exported, first, sizeof(first), reply_one, sizeof(reply_one)) != STYXRPC_OK ||
        styxrpc_managed_open(&b, &exported, second, sizeof(second), reply_two, sizeof(reply_two)) != STYXRPC_OK ||
        styxrpc_managed_write(&a, 0, request, 1, scratch, sizeof(scratch)) != STYXRPC_MORE || state.begins ||
        styxrpc_managed_write(&a, 1, request + 1, length - 1, scratch, sizeof(scratch)) != STYXRPC_MORE ||
        styxrpc_managed_read(&a, 0, scratch, sizeof(scratch), &received) != STYXRPC_MORE || received ||
        styxrpc_managed_write(&b, 0, request, length, scratch, sizeof(scratch)) != STYXRPC_MORE ||
        state.begins != 2)
    {
        fputs("failed to stage two independent calls\n", stderr);
        return 1;
    }
    styxrpc_managed_close(&a);
    styxrpc_managed_close(&a);
    StyxRpcManagedReply decoded;
    const StyxRpcOutput output = {.result = value, .result_size = value_size};
    if (state.cancels != 1 || styxrpc_managed_finish(&a, &output, scratch, sizeof(scratch)) != STYXRPC_INVALID ||
        styxrpc_managed_finish(&b, &output, scratch, sizeof(scratch)) != STYXRPC_OK ||
        styxrpc_managed_read(&b, 0, scratch, sizeof(scratch), &received) != STYXRPC_OK ||
        styxrpc_managed_reply(exported.signature, &exported.schema, request, length,
            scratch, received, &decoded) != STYXRPC_OK || decoded.void_result || decoded.changed ||
        decoded.result.size != value_size || memcmp(decoded.result.payload, value, value_size))
    {
        fputs("failed to cancel or decode the surviving call\n", stderr);
        return 1;
    }
    styxrpc_managed_close(&b);
    scratch[received - 1] ^= 0x80;
    if (state.cancels != 1 || styxrpc_managed_reply(exported.signature, &exported.schema,
        request, length, scratch, received, &decoded) != STYXRPC_INVALID)
    {
        fputs("corrupt reply accepted or completed call cancelled\n", stderr);
        return 1;
    }
    return 0;
}
