/* Copyright (c) 2026 bhgv
 * EN: Per-fid scalar request and reply state for a Styx function file.
 * RU: Состояние скалярного запроса и ответа файла функции на каждый fid.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "scalar_function.h"
#include <stdlib.h>
#include <string.h>

typedef struct FunctionFid
{
    StyxScalarFunction function; /* Copied configuration, with borrowed method context. */
    size_t size;                 /* Reply length, zero before an accepted request. */
    unsigned char reply[STYX_SCALAR_RECORD_CAPACITY]; /* Immutable encoded response. */
} FunctionFid;


/* EN/RU: Install private state on the current server fid for ORDWR only.
 * Закрепить отдельное состояние за текущим fid сервера только для ORDWR. */
char *styxscalarfunctionopen(Styxserver *server, int mode, const StyxScalarFunction *function)
{
    if (mode != ORDWR || !function || !function->method || !function->request_tag ||
        !function->reply_tag || function->request_tag > UINT32_MAX || function->reply_tag > UINT32_MAX)
    {
        return Eperm;
    }
    FunctionFid *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return Enomem;
    }
    fid->function = *function;
    if (styxsetaux(server, fid) < 0)
    {
        free(fid);
        return Eperm;
    }
    return nil;
}


/* EN/RU: Validate one whole request at offset zero and store the method's reply.
 * Проверить полный запрос по смещению ноль и сохранить ответ метода. */
char *styxscalarfunctionwrite(void *state, const char *bytes, ulong count, vlong offset)
{
    FunctionFid *fid = state;
    if (!fid || fid->size || offset || count > STYX_SCALAR_RECORD_CAPACITY)
    {
        return Ebadarg;
    }
    const StyxScalarFunction *function = &fid->function;
    return styxscalarrespond(function->request_tag, bytes, count, function->reply_tag,
        function->method, function->state, fid->reply, sizeof(fid->reply), &fid->size) ? nil : Ebadarg;
}


/* EN/RU: Copy a reply slice to bytes and report its count; reject reads before a request.
 * Копировать часть ответа в bytes и вернуть размер; отказать до получения запроса. */
char *styxscalarfunctionread(void *state, char *bytes, ulong *count, vlong offset)
{
    FunctionFid *fid = state;
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
