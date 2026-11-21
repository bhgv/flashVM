/* Copyright (c) 2026 bhgv
 * EN: Per-fid scalar request and reply state for a Styx function file.
 * RU: Состояние скалярного запроса и ответа файла функции на каждый fid.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "scalar_function.h"
#include "scalar_verify.h"
#include "call_scope.h"
#include "cancel.h"
#include "flashvm/srecord.h"
#include <inttypes.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct FunctionFid
{
    StyxScalarFunction function; /* Copied configuration, with borrowed method context. */
    int report_transport_failure; /* Interpret negative service status as missing reply. */
    DVM_Cancel *cancel; /* Owned token for this exact admitted call. */
    int *workers; /* Server-owned active job counter, retained through dispatcher join. */
    int *busy; /* Export-owned mailbox gate. */
    unsigned references; /* Fid and optional worker references. */
    int closed; /* Atomic revocation of this fid. */
    int running; /* Async request accepted, written only by the dispatcher. */
    int ready; /* Atomic publication of the worker's reply or failure. */
    char *failure; /* Worker result published with ready. */
    int context_written; /* One context selector precedes request staging. */
    uint32_t context_pid; /* Zero selects the PID in the scoped request. */
    size_t staged_size; /* Bytes of the legacy request awaiting the end record. */
    size_t write_offset; /* Exact offset after the staged envelope. */
    unsigned char staged[STYX_FUNCTION_FRAME_CAPACITY + STYX_SCOPED_CALL_OVERHEAD]; /* Request owned by this fid. */
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
    if (fn->scoped_call_method && (!fn->program_root || !fn->dynamic_shape || fn->call_method || fn->ref_pair_method))
    {
        return 0;
    }
    if (fn->ref_pair_method || fn->scoped_ref_pair_method)
    {
        return !fn->method && !fn->caller_method && !fn->program_method && !fn->tuple_method &&
            !fn->ref_method && !fn->call_method && !fn->scoped_call_method &&
            (fn->program_root ? fn->scoped_ref_pair_method && !fn->ref_pair_method :
                fn->ref_pair_method && !fn->scoped_ref_pair_method) &&
            fn->request_tag == STYX_REF_PAIR_REQUEST_TAG && fn->reply_tag == STYX_REF_PAIR_REPLY_TAG;
    }
    if (fn->call_method || fn->scoped_call_method)
    {
        size_t args, result;
        int profile = fn->dynamic_shape ?
            fn->request_tag == STYX_DYNAMIC_REQUEST_TAG && fn->reply_tag == STYX_DYNAMIC_REPLY_TAG &&
                styxshapecells(fn->dynamic_shape, &args, &result) :
            fn->request_tag == STYX_CALL_LIST_REQUEST_TAG && fn->reply_tag == STYX_CALL_LIST_REPLY_TAG;
        return profile && !fn->method && !fn->caller_method && !fn->program_method && !fn->tuple_method &&
            !fn->ref_method && (!!fn->program_root == !!fn->scoped_call_method);
    }
    if (fn->ref_method)
    {
        return !fn->method && !fn->caller_method && !fn->program_method && !fn->tuple_method &&
            fn->reply_tag == STYX_REF_REPLY_TAG &&
            (fn->request_tag == STYX_REF_REQUEST_TAG ||
                (fn->program_root && fn->request_tag == STYX_REF_CALLER_TAG));
    }
    if (fn->tuple_method)
    {
        return !fn->method && !fn->caller_method && !fn->program_method &&
            fn->reply_tag == STYX_TUPLE_REPLY_TAG &&
            (fn->program_root ? fn->request_tag == STYX_TUPLE_CALLER_TAG :
                fn->request_tag == STYX_TUPLE_REQUEST_TAG);
    }
    uint32_t signature = styxscalar_request_signature(fn->request_tag);
    if (signature == DVM_TUPLE_SIGNATURE || signature == DVM_REF_SIGNATURE || signature == DVM_CALL_LIST_SIGNATURE ||
        signature == DVM_REF_PAIR_SIGNATURE || fn->request_tag == STYX_DYNAMIC_REQUEST_TAG ||
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
char *styxscalarfunctionopen(Styxserver *server, int mode, const StyxScalarFunction *function, int report_transport_failure, int *workers, int *busy)
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
    fid->references = 1;
    fid->workers = workers;
    fid->busy = busy;
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
static DVM_Provider *capture_self(DVM_Provider *root, uint32_t pid, DVM_Cancel *cancel)
{
    char path[sizeof(".prog/4294967295")];
    snprintf(path, sizeof(path), ".prog/%" PRIu32, pid);
    DVM_Provider *self = dvm_provider_capture(root, path);
    FVM_FileInfo info;
    if (self && dvm_provider_info(self, ".", &info) == FVM_OK && info.directory &&
        dvm_provider_set_styx_caller(self, root, pid) &&
        (!cancel || dvm_provider_set_call_cancel(self, cancel)))
    {
        return self;
    }
    dvm_provider_release(self);
    return NULL;
}


/* EN: Decode a complete tuple before dispatch, then encode its result or error.
 * RU: Проверить полный кортеж перед вызовом, затем закодировать результат или ошибку. */
static char *tuple_call(FunctionFid *fid, const void *bytes, size_t count, uint32_t scoped_pid)
{
    const StyxScalarFunction *fn = &fid->function;
    StyxTuple argument;
    StyxRefReply result = {0};
    uint32_t pid;
    if (!styxtupleread(fn->request_tag, bytes, count, &pid, &argument))
    {
        return Ebadarg;
    }
    pid = scoped_pid ? scoped_pid : pid;
    DVM_Provider *self = pid ? capture_self(fn->program_root, pid, fid->cancel) : NULL;
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


/* EN: Validate the full argument list before invoking and retain only the encoded fid reply.
 * RU: Проверить весь список аргументов перед вызовом, сохранить только ответ в fid. */
static char *call_list(FunctionFid *fid, const void *bytes, size_t count, uint32_t pid)
{
    StyxCallValues arguments, result = {0};
    const StyxScalarFunction *fn = &fid->function;
    const char *input_shape = fn->dynamic_shape ? fn->dynamic_shape->arguments : styxcall_list_args;
    const char *output_shape = fn->dynamic_shape ? fn->dynamic_shape->result : styxcall_list_result;
    if (!styxcallrecordread(fn->request_tag, input_shape, bytes, count, &arguments))
    {
        return Ebadarg;
    }
    DVM_Provider *self = pid ? capture_self(fn->program_root, pid, fid->cancel) : NULL;
    if (pid && !self)
    {
        return Eperm;
    }
    fid->submitted = 1;
    int outcome = self ? fn->scoped_call_method(fn->state, self, &arguments, &result) :
        fn->call_method(fn->state, &arguments, &result);
    dvm_provider_release(self);
    if (outcome < 0 && fid->report_transport_failure)
    {
        return Ehungup;
    }
    int encoded = outcome == 1 ?
        styxcallrecordwrite(fn->reply_tag, output_shape, &result,
            fid->reply, sizeof(fid->reply), &fid->size) :
        styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0, fid->reply, sizeof(fid->reply), &fid->size);
    return encoded ? nil : Ebadarg;
}


/* EN: Run one admitted pair and retain only its reply on fid. The callback
 * may update cells and return, while object count and order belong to the request.
 * RU: Выполнить один принятый вызов и сохранить ответ в fid. Обработчик меняет
 * поля и результат, а количество и порядок объектов задаёт запрос. */
static char *ref_pair_call(FunctionFid *fid, const void *bytes, size_t count, uint32_t pid)
{
    StyxRefPair pair;
    if (!styxrefpairread(STYX_REF_PAIR_REQUEST_TAG, bytes, count, &pair))
    {
        return Ebadarg;
    }
    const StyxScalarFunction *fn = &fid->function;
    DVM_Provider *self = pid ? capture_self(fn->program_root, pid, fid->cancel) : NULL;
    if (pid && !self)
    {
        return Eperm;
    }
    size_t objects = pair.count;
    fid->submitted = 1;
    int outcome = self ? fn->scoped_ref_pair_method(fn->state, self, &pair) : fn->ref_pair_method(fn->state, &pair);
    dvm_provider_release(self);
    if (outcome < 0 && fid->report_transport_failure)
    {
        return Ehungup;
    }
    int encoded = outcome == 1 ?
        pair.count == objects && styxrefpairwrite(STYX_REF_PAIR_REPLY_TAG, &pair,
            fid->reply, sizeof(fid->reply), &fid->size) :
        styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0, fid->reply, sizeof(fid->reply), &fid->size);
    return encoded ? nil : Ebadarg;
}


/* EN: Validate and dispatch one complete legacy-format request.
 * RU: Проверить и передать один полный запрос прежнего формата. */
static char *dispatch(FunctionFid *fid, const void *bytes, size_t count)
{
    const StyxScalarFunction *function = &fid->function;
    uint32_t scoped_pid = 0;
    const void *request = bytes;
    size_t length = count;
    int envelope = function->scoped_call_method || function->scoped_ref_pair_method ||
        (function->ref_method && function->program_root && function->request_tag == STYX_REF_REQUEST_TAG);
    if ((envelope && !styxcall_scope_read(bytes, count, &scoped_pid, &request, &length)) ||
        (!envelope && count > STYX_FUNCTION_FRAME_CAPACITY) ||
        (fid->context_written && (!envelope || (fid->context_pid && fid->context_pid != scoped_pid))))
    {
        return Ebadarg;
    }
    if (function->ref_pair_method || function->scoped_ref_pair_method)
    {
        return ref_pair_call(fid, request, length, scoped_pid);
    }
    if (function->call_method || function->scoped_call_method)
    {
        return call_list(fid, request, length, scoped_pid);
    }
    if (function->tuple_method || function->ref_method)
    {
        return tuple_call(fid, request, length, scoped_pid);
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
    DVM_Provider *self = function->program_method ? capture_self(function->program_root, pid, fid->cancel) : NULL;
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


/* EN: Drop one worker or fid reference; free private buffers after the final drop.
 * RU: Снять ссылку задачи или fid; освободить буферы после последней ссылки. */
static void release_fid(FunctionFid *fid)
{
    if (__atomic_sub_fetch(&fid->references, 1, __ATOMIC_ACQ_REL) == 0)
    {
        dvm_cancel_release(fid->cancel);
        free(fid);
    }
}


/* EN: Revoke the fid and let an admitted native worker retire independently.
 * RU: Отозвать fid и дать принятой нативной задаче завершиться самостоятельно. */
void styxscalarfunctionclose(void *state)
{
    FunctionFid *fid = state;
    if (fid)
    {
        __atomic_store_n(&fid->closed, 1, __ATOMIC_RELEASE);
        dvm_cancel_request(fid->cancel);
        release_fid(fid);
    }
}


/* EN: Wait for the export mailbox, skip revoked queued work and publish the result.
 * RU: Дождаться приёмника экспорта, пропустить отозванную задачу и выдать результат. */
static void *run_async(void *state)
{
    FunctionFid *fid = state;
    int *workers = fid->workers;
    const struct timespec pause = {0, 1000000};
    /* EN: Scoped exports with worker handoff have independent reply routes.
     * RU: У экспортов с передачей вызова исполнителю отдельные маршруты ответа. */
    int independent = fid->function.program_root && (fid->function.dynamic_shape ||
        fid->function.ref_method || fid->function.scoped_ref_pair_method);
    while (!__atomic_load_n(&fid->closed, __ATOMIC_ACQUIRE))
    {
        int idle = 0;
        if (independent || __atomic_compare_exchange_n(fid->busy, &idle, 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        {
            if (!__atomic_load_n(&fid->closed, __ATOMIC_ACQUIRE))
            {
                fid->failure = dispatch(fid, fid->staged, fid->staged_size);
                __atomic_store_n(&fid->ready, 1, __ATOMIC_RELEASE);
            }
            if (!independent)
            {
                __atomic_store_n(fid->busy, 0, __ATOMIC_RELEASE);
            }
            break;
        }
        nanosleep(&pause, NULL);
    }
    release_fid(fid);
    __atomic_sub_fetch(workers, 1, __ATOMIC_RELEASE);
    return NULL;
}


/* EN: Schedule one bounded job without blocking the Styx dispatcher.
 * RU: Принять одну задачу в пределах лимита без блокировки диспетчера Styx. */
static char *schedule(FunctionFid *fid)
{
    if (!fid->cancel && !(fid->cancel = dvm_cancel_create()))
    {
        return Enomem;
    }
    pthread_attr_t attributes;
    if (__atomic_load_n(fid->workers, __ATOMIC_ACQUIRE) >= 16 || pthread_attr_init(&attributes))
    {
        return Eperm;
    }
    int failed = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
    if (!failed)
    {
        pthread_t worker;
        __atomic_add_fetch(&fid->references, 1, __ATOMIC_RELAXED);
        __atomic_add_fetch(fid->workers, 1, __ATOMIC_RELAXED);
        failed = pthread_create(&worker, &attributes, run_async, fid);
        if (failed)
        {
            __atomic_sub_fetch(fid->workers, 1, __ATOMIC_RELEASE);
            release_fid(fid);
        }
    }
    pthread_attr_destroy(&attributes);
    fid->running = !failed;
    return failed ? Eperm : nil;
}


/* EN: Accept legacy requests or a staged request followed by an explicit end record.
 * RU: Принять прежний запрос либо отложенный запрос с отдельной записью конца. */
char *styxscalarfunctionwrite(void *state, const char *bytes, ulong count, vlong offset)
{
    FunctionFid *fid = state;
    if (!fid || fid->running || fid->submitted || offset < 0 || (uint64_t)offset != fid->write_offset)
    {
        return Ebadarg;
    }
    FVM_SRecord record;
    size_t used;
    int complete = count <= sizeof(fid->staged) + STYX_SCOPED_CALL_OVERHEAD &&
        fvm_srecord_read(bytes, count, sizeof(fid->staged) + STYX_SCOPED_CALL_OVERHEAD,
            &record, &used) == FVM_S_OK && used == count;
    if (fid->staged_size)
    {
        if (!complete || record.tag != STYX_CALL_END_TAG || record.size)
        {
            return Ebadarg;
        }
        if (fid->workers)
        {
            return fid->context_written ? schedule(fid) : Ebadarg;
        }
        return dispatch(fid, fid->staged, fid->staged_size);
    }
    if (complete && record.tag == STYX_CALL_CONTEXT_TAG && !offset)
    {
        if (!styxcall_context(record.payload, record.size, &fid->context_pid))
        {
            return Ebadarg;
        }
        fid->context_written = 1;
        fid->write_offset = count;
        return nil;
    }
    if (complete && record.tag == STYX_CALL_STAGE_TAG)
    {
        if (!record.size || record.size > sizeof(fid->staged))
        {
            return Ebadarg;
        }
        memcpy(fid->staged, record.payload, record.size);
        fid->staged_size = record.size;
        fid->write_offset += count;
        return nil;
    }
    return !fid->workers && !fid->context_written && count <= sizeof(fid->staged) ? dispatch(fid, bytes, count) : Ebadarg;
}


/* EN/RU: Copy a reply slice to bytes and report its count; reject reads before a request.
 * Копировать часть ответа в bytes и вернуть размер; отказать до получения запроса. */
char *styxscalarfunctionread(void *state, char *bytes, ulong *count, vlong offset)
{
    FunctionFid *fid = state;
    if (fid && fid->running)
    {
        if (!__atomic_load_n(&fid->ready, __ATOMIC_ACQUIRE))
        {
            *count = 0;
            return offset == 0 ? nil : Ebadarg;
        }
        if (fid->failure)
        {
            return fid->failure;
        }
    }
    if (fid && fid->staged_size && !fid->submitted && offset == 0)
    {
        *count = 0;
        return nil;
    }
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
