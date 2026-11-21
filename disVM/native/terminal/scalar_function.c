/* Copyright (c) 2026 bhgv
 * EN: Per-fid scalar request and reply state for a Styx function file.
 * RU: Состояние скалярного запроса и ответа файла функции на каждый fid.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "scalar_function.h"
#include "scalar_verify.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct FunctionFid
{
    StyxScalarFunction function; /* Copied configuration, with borrowed method context. */
    int report_transport_failure; /* Interpret negative service status as missing reply. */
    int submitted;              /* A validated request reached the method. */
    size_t size;                 /* Reply length, zero before an accepted request. */
    unsigned char reply[STYX_FUNCTION_FRAME_CAPACITY]; /* Immutable encoded response. */
} FunctionFid;


/* EN: Verify exactly one compatible typed or legacy callback.
 * RU: Проверить ровно один совместимый типизированный или прежний callback. */
int styxscalarfunctionvalid(const StyxScalarFunction *fn)
{
    if (!fn || !fn->request_tag || !fn->reply_tag ||
        fn->request_tag > UINT32_MAX || fn->reply_tag > UINT32_MAX)
    {
        return 0;
    }
    if (fn->ref_method)
    {
        return !fn->method && !fn->caller_method && !fn->program_method && !fn->tuple_method &&
            fn->reply_tag == STYX_REF_REPLY_TAG &&
            (fn->program_root ? fn->request_tag == STYX_REF_CALLER_TAG :
                fn->request_tag == STYX_REF_REQUEST_TAG);
    }
    if (fn->tuple_method)
    {
        return !fn->method && !fn->caller_method && !fn->program_method &&
            fn->reply_tag == STYX_TUPLE_REPLY_TAG &&
            (fn->program_root ? fn->request_tag == STYX_TUPLE_CALLER_TAG :
                fn->request_tag == STYX_TUPLE_REQUEST_TAG);
    }
    uint32_t signature = styxscalar_request_signature(fn->request_tag);
    if (signature == DVM_TUPLE_SIGNATURE || signature == DVM_REF_SIGNATURE ||
        !!fn->program_root != !!fn->program_method || (fn->caller_method && fn->program_method))
    {
        return 0;
    }
    int scoped = fn->caller_method || fn->program_method;
    int caller_tag = fn->request_tag == STYX_FUNCTION_CALLER_TAG || fn->request_tag == STYX_FUNCTION_BIG_CALLER_TAG;
    return scoped ? !fn->method && caller_tag && fn->reply_tag == STYX_FUNCTION_REPLY_TAG :
        fn->method && !caller_tag;
}


/* EN/RU: Install private state on the current server fid for ORDWR only.
 * Закрепить отдельное состояние за текущим fid сервера только для ORDWR. */
char *styxscalarfunctionopen(Styxserver *server, int mode, const StyxScalarFunction *function, int report_transport_failure)
{
    if (mode != ORDWR || !styxscalarfunctionvalid(function))
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


/* EN: Capture the selected process from a borrowed reverse root, returning an owned directory.
 * RU: Захватить выбранный процесс из заимствованного обратного корня и вернуть свой каталог. */
static DVM_Provider *capture_self(DVM_Provider *root, uint32_t pid)
{
    char path[sizeof(".prog/4294967295")];
    snprintf(path, sizeof(path), ".prog/%" PRIu32, pid);
    DVM_Provider *self = dvm_provider_capture(root, path);
    FVM_FileInfo info;
    if (self && dvm_provider_info(self, ".", &info) == FVM_OK && info.directory)
    {
        return self;
    }
    dvm_provider_release(self);
    return NULL;
}


/* EN: Decode a complete tuple before dispatch, then encode its result or error.
 * RU: Проверить полный кортеж перед вызовом, затем закодировать результат или ошибку. */
static char *tuple_call(FunctionFid *fid, const char *bytes, size_t count)
{
    const StyxScalarFunction *fn = &fid->function;
    StyxTuple argument;
    StyxRefReply result = {0};
    uint32_t pid;
    if (!styxtupleread(fn->request_tag, bytes, count, &pid, &argument))
    {
        return Ebadarg;
    }
    DVM_Provider *self = pid ? capture_self(fn->program_root, pid) : NULL;
    if (pid && !self)
    {
        return Eperm;
    }
    fid->submitted = 1;
    result.updated = argument;
    int outcome = fn->ref_method ? fn->ref_method(fn->state, self, &argument, &result) :
        fn->tuple_method(fn->state, self, &argument, &result.result);
    dvm_provider_release(self);
    if (outcome < 0 && fid->report_transport_failure)
    {
        return Ehungup;
    }
    int encoded = outcome == 1 ?
        (fn->ref_method ? styxrefreplywrite(&result, fid->reply, sizeof(fid->reply), &fid->size) :
            styxtuplewrite(fn->reply_tag, 0, &result.result, fid->reply, sizeof(fid->reply), &fid->size)) :
        styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0, fid->reply, sizeof(fid->reply), &fid->size);
    return encoded ? nil : Ebadarg;
}


/* EN/RU: Validate one whole request at offset zero and store the method's reply.
 * Проверить полный запрос по смещению ноль и сохранить ответ метода. */
char *styxscalarfunctionwrite(void *state, const char *bytes, ulong count, vlong offset)
{
    FunctionFid *fid = state;
    if (!fid || fid->submitted || offset || count > STYX_FUNCTION_FRAME_CAPACITY)
    {
        return Ebadarg;
    }
    const StyxScalarFunction *function = &fid->function;
    if (function->tuple_method || function->ref_method)
    {
        return tuple_call(fid, bytes, count);
    }
    int scoped = function->caller_method || function->program_method;
    uint32_t signature = styxscalar_request_signature(function->request_tag);
    int narrow = signature == DVM_SCALAR_INT_SIGNATURE && function->reply_tag == STYX_FUNCTION_REPLY_TAG;
    StyxScalarKind kind;
    int64_t argument;
    uint32_t pid = 0;
    int admitted = scoped && signature == DVM_SCALAR_BIG_SIGNATURE ?
        styxscalarbigcallerread(bytes, count, &pid, &argument) : scoped ?
        styxscalarcallerread(bytes, count, &pid, &argument) :
        styxscalarread(function->request_tag, bytes, count, &kind, &argument) && kind == STYX_SCALAR_CALL;
    if (!admitted || (narrow && (argument < INT32_MIN || argument > INT32_MAX)))
    {
        return Ebadarg;
    }
    DVM_Provider *self = function->program_method ? capture_self(function->program_root, pid) : NULL;
    if (function->program_method && !self)
    {
        return Eperm;
    }
    int64_t value = 0;
    fid->submitted = 1;
    int outcome = self ? function->program_method(function->state, self, argument, &value) :
        function->caller_method ? function->caller_method(function->state, pid, argument, &value) :
        function->method(function->state, argument, &value);
    dvm_provider_release(self);
    if (outcome < 0 && fid->report_transport_failure)
    {
        return Ehungup;
    }
    int success = outcome == 1;
    if (narrow && (value < INT32_MIN || value > INT32_MAX))
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
