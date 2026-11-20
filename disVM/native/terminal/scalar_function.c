/* Copyright (c) 2026 bhgv
 * EN: Per-fid scalar request and reply state for a Styx function file.
 * RU: Состояние скалярного запроса и ответа файла функции на каждый fid.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "scalar_function.h"
#include "scalar_verify.h"
#include <stdlib.h>
#include <string.h>

typedef struct FunctionFid
{
    StyxScalarFunction function; /* Copied configuration, with borrowed method context. */
    int report_transport_failure; /* Interpret negative service status as missing reply. */
    int submitted;              /* A validated request reached the method. */
    size_t size;                 /* Reply length, zero before an accepted request. */
    unsigned char reply[STYX_SCALAR_RECORD_CAPACITY]; /* Immutable encoded response. */
} FunctionFid;


/* EN/RU: Install private state on the current server fid for ORDWR only.
 * Закрепить отдельное состояние за текущим fid сервера только для ORDWR. */
char *styxscalarfunctionopen(Styxserver *server, int mode, const StyxScalarFunction *function, int report_transport_failure)
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
    fid->report_transport_failure = report_transport_failure;
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
    if (!fid || fid->submitted || offset || count > STYX_SCALAR_RECORD_CAPACITY)
    {
        return Ebadarg;
    }
    const StyxScalarFunction *function = &fid->function;
    int typed = function->request_tag == STYX_FUNCTION_REQUEST_TAG &&
        function->reply_tag == STYX_FUNCTION_REPLY_TAG;
    StyxScalarKind kind;
    int64_t argument;
    if (!styxscalarread(function->request_tag, bytes, count, &kind, &argument) ||
        kind != STYX_SCALAR_CALL || (typed && (argument < INT32_MIN || argument > INT32_MAX)))
    {
        return Ebadarg;
    }
    int64_t value = 0;
    fid->submitted = 1;
    int outcome = function->method(function->state, argument, &value);
    if (outcome < 0 && fid->report_transport_failure)
    {
        return Ehungup;
    }
    int success = outcome == 1;
    if (typed && (value < INT32_MIN || value > INT32_MAX))
    {
        success = 0;
    }
    return styxscalarwrite(function->reply_tag, success ? STYX_SCALAR_VALUE : STYX_SCALAR_ERROR,
        value, fid->reply, sizeof(fid->reply), &fid->size) ? nil : Ebadarg;
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
