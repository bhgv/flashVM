/* Copyright (c) 2026 bhgv
 * EN: Check standalone callback framing, compiled profile and per-fid isolation.
 * RU: Проверка самостоятельного кодека, скомпилированного профиля и изоляции fid.
 */
#include "styxrpc/rpc.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include <string.h>


/* EN: Return argument plus one, remembering the number of callback executions.
 * RU: Вернуть аргумент плюс один и запомнить число запусков колбэка. */
static int invoke(void *state, int64_t argument, int64_t *result)
{
    ++*(unsigned *)state;
    *result = argument + 1;
    return 1;
}


/* EN: Stage fragments, refuse corruption and replay, and verify the exact legacy reply.
 * RU: Принять фрагменты, отклонить повреждение и повтор, сверить исходный ответ. */
int main(void)
{
    unsigned calls = 0;
    StyxRpcExport exported = {"sample", STYXRPC_INT_SIGNATURE, invoke, &calls};
    unsigned char profile[128], request[32], reply[32];
    size_t length, got, consumed;
    StyxRpcCall first, second;
    int64_t value;
    if (styxrpc_profile(&exported, 1, profile, sizeof(profile), &length) != STYXRPC_OK ||
        !styxscalar_profile_match(profile, length, "sample", DVM_SCALAR_INT_SIGNATURE, NULL) ||
        styxrpc_open(&first, &exported) != STYXRPC_OK ||
        styxrpc_open(&second, &exported) != STYXRPC_OK ||
        styxrpc_request(exported.signature, 41, request, sizeof(request), &length) != STYXRPC_OK ||
        styxrpc_write(&first, 0, request, 2) != STYXRPC_MORE ||
        styxrpc_read(&first, 0, reply, sizeof(reply), &got) != STYXRPC_MORE ||
        calls || styxrpc_write(&first, 2, request + 2, length - 2) != STYXRPC_OK ||
        styxrpc_write(&first, 0, request, length) != STYXRPC_INVALID ||
        styxrpc_read(&first, 0, reply, 1, &got) != STYXRPC_OK || got != 1 ||
        styxrpc_read(&first, 1, reply + 1, sizeof(reply) - 1, &consumed) != STYXRPC_OK ||
        styxrpc_reply(exported.signature, reply, consumed + 1, &value) != STYXRPC_OK || value != 42)
    {
        return 1;
    }
    request[length - 1] ^= 1;
    return styxrpc_write(&second, 0, request, length) != STYXRPC_INVALID || calls != 1;
}
