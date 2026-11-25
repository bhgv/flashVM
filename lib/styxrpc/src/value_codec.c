/* Copyright (c) 2026 bhgv
 * EN: Encode and validate the portable scalar children used by RPC and UniBus.
 * RU: Кодирование и проверка переносимых скалярных записей RPC и UniBus.
 */
#include "styxrpc/managed.h"
#include "scalar_record.h"
#include <float.h>
#include <string.h>


/* EN/RU: See managed.h. / См. managed.h. */
StyxRpcStatus styxrpc_value_integer(unsigned long tag, int64_t value, void *bytes,
    size_t capacity, size_t *used)
{
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    int valid = tag == STYXRPC_VALUE_BIG ||
        (tag == STYXRPC_VALUE_INT && value >= INT32_MIN && value <= INT32_MAX) ||
        (tag == STYXRPC_VALUE_BYTE && value >= 0 && value <= UINT8_MAX);
    return valid && bytes && styxscalarwrite(tag, STYX_SCALAR_VALUE, value, bytes, capacity, used) ?
        STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See managed.h. / См. managed.h. */
StyxRpcStatus styxrpc_value_get_integer(unsigned long tag, const void *bytes, size_t size, int64_t *value)
{
    StyxScalarKind kind;
    int64_t decoded;
    if (!value || !styxscalarread(tag, bytes, size, &kind, &decoded) || kind != STYX_SCALAR_VALUE ||
        !(tag == STYXRPC_VALUE_BIG ||
          (tag == STYXRPC_VALUE_INT && decoded >= INT32_MIN && decoded <= INT32_MAX) ||
          (tag == STYXRPC_VALUE_BYTE && decoded >= 0 && decoded <= UINT8_MAX)))
    {
        return STYXRPC_INVALID;
    }
    *value = decoded;
    return STYXRPC_OK;
}


/* EN: Require the native representation used by the compiled Dis real profile.
 * RU: Требовать представление компилированного профиля real Dis. */
static int binary64(void)
{
    uint64_t bits;
    double one = 1.0;
    if (sizeof(double) != sizeof(bits) || FLT_RADIX != 2 || DBL_MANT_DIG != 53 || DBL_MAX_EXP != 1024)
    {
        return 0;
    }
    memcpy(&bits, &one, sizeof(bits));
    return bits == UINT64_C(0x3ff0000000000000);
}


/* EN/RU: See managed.h. / См. managed.h. */
StyxRpcStatus styxrpc_value_real(double value, void *bytes, size_t capacity, size_t *used)
{
    uint64_t bits;
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    if (!binary64() || !bytes)
    {
        return STYXRPC_INVALID;
    }
    memcpy(&bits, &value, sizeof(bits));
    int64_t word = bits <= INT64_MAX ? (int64_t)bits : -1 - (int64_t)(UINT64_MAX - bits);
    return styxscalarwrite(STYXRPC_VALUE_REAL, STYX_SCALAR_VALUE, word, bytes, capacity, used) ?
        STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See managed.h. / См. managed.h. */
StyxRpcStatus styxrpc_value_get_real(const void *bytes, size_t size, double *value)
{
    StyxScalarKind kind;
    int64_t word;
    if (!value || !binary64() || !styxscalarread(STYXRPC_VALUE_REAL, bytes, size, &kind, &word) ||
        kind != STYX_SCALAR_VALUE)
    {
        return STYXRPC_INVALID;
    }
    uint64_t bits = (uint64_t)word;
    memcpy(value, &bits, sizeof(bits));
    return STYXRPC_OK;
}
