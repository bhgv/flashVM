/* Copyright (c) 2026 bhgv
 * EN: Exchange a function call with the shared Dis owner service over local inboxes.
 * RU: Обмен вызовом с общим сервисом владельца Dis через локальные приёмники.
 */
#include <lib9.h>
#include "scalar_function.h"
#include "scalar_function_owner.h"
#include "record_write.h"
#include "scalar_verify.h"
#include <time.h>


/* EN: Exchange one complete request and capture the reply, at most once.
 * RU: Обменять полную запись и захватить ответ без повторной отправки. */
static int exchange(StyxScalarFunctionOwner *owner, unsigned char *record, size_t size, size_t *received)
{
    DVM_VfsHandle reader = {0};
    struct timespec start;
    if (owner->retired || clock_gettime(CLOCK_MONOTONIC, &start) ||
        dvm_provider_open(owner->reply, ".", "r", &reader) != FVM_OK)
    {
        return 0;
    }
    int ready = 0;
    if (styxterminalrecordwrite(owner->request, record, size) == DVM_LAUNCH_DELIVERED)
    {
        struct timespec now;
        while (!clock_gettime(CLOCK_MONOTONIC, &now) &&
            (now.tv_sec - start.tv_sec) * INT64_C(1000) + (now.tv_nsec - start.tv_nsec) / 1000000 < 3000)
        {
            DVM_IoResult got = dvm_vfs_read_chunk(&reader, record, STYX_FUNCTION_FRAME_CAPACITY);
            if (got.status == FVM_OK && (got.flags & DVM_IO_RECORD) && !(got.flags & DVM_IO_UNCERTAIN))
            {
                *received = got.count;
                ready = 1;
                break;
            }
            if (got.stop != DVM_IO_AGAIN || got.count || (got.flags & DVM_IO_UNCERTAIN))
            {
                break;
            }
            const struct timespec delay = {0, 10000000};
            nanosleep(&delay, NULL);
        }
    }
    dvm_vfs_close(&reader);
    return ready;
}


/* EN: Retire an ambiguous route before accepting any later request.
 * RU: Закрыть маршрут с неопределённым исходом до следующего запроса. */
static int retire(StyxScalarFunctionOwner *owner)
{
    owner->retired = 1;
    styxunibusmailboxseal(owner->request);
    return -1;
}


/* EN: Exchange the existing narrow or wide scalar with the owner instance.
 * RU: Обменять прежний узкий или широкий скаляр с экземпляром владельца. */
int styxscalarfunctiondispatch(void *state, int64_t argument, int64_t *value)
{
    StyxScalarFunctionOwner *owner = state;
    unsigned char record[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size, received;
    StyxScalarKind kind;
    if (!styxscalarwrite(owner->request_tag, STYX_SCALAR_CALL, argument, record, sizeof(record), &size) ||
        !exchange(owner, record, size, &received) ||
        !styxscalarread(STYX_FUNCTION_REPLY_TAG, record, received, &kind, value) ||
        (kind != STYX_SCALAR_ERROR && kind != STYX_SCALAR_VALUE))
    {
        return retire(owner);
    }
    return kind == STYX_SCALAR_VALUE;
}


/* EN: Exchange one composed value with the owner's linked Dis export.
 * RU: Обменять составное значение со связанным экспортом Dis владельца. */
static int tuple_dispatch(void *state, DVM_Provider *self, const StyxTuple *argument, StyxTuple *value)
{
    (void)self;
    StyxScalarFunctionOwner *owner = state;
    unsigned char record[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size, received;
    StyxScalarKind kind;
    int64_t code;
    uint32_t pid;
    if (styxtuplewrite(STYX_TUPLE_REQUEST_TAG, 0, argument, record, sizeof(record), &size) &&
        exchange(owner, record, size, &received))
    {
        if (styxtupleread(STYX_TUPLE_REPLY_TAG, record, received, &pid, value))
        {
            return 1;
        }
        if (styxscalarread(STYX_TUPLE_REPLY_TAG, record, received, &kind, &code) && kind == STYX_SCALAR_ERROR)
        {
            return 0;
        }
    }
    return retire(owner);
}


/* EN: Exchange the input fields and two validated output snapshots with owner.
 * RU: Обменять поля аргумента и два проверенных снимка с владельцем. */
static int ref_dispatch(void *state, DVM_Provider *self, const StyxTuple *argument, StyxRefReply *value)
{
    (void)self;
    StyxScalarFunctionOwner *owner = state;
    unsigned char record[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size, received;
    StyxScalarKind kind;
    int64_t code;
    if (styxtuplewrite(STYX_REF_REQUEST_TAG, 0, argument, record, sizeof(record), &size) &&
        exchange(owner, record, size, &received))
    {
        if (styxrefreplyread(record, received, value))
        {
            return 1;
        }
        if (styxscalarread(STYX_REF_REPLY_TAG, record, received, &kind, &code) && kind == STYX_SCALAR_ERROR)
        {
            return 0;
        }
    }
    return retire(owner);
}


/* EN/RU: See scalar_function_owner.h. / См. scalar_function_owner.h. */
int styxscalarownerconfigure(StyxScalarFunctionOwner *owner, StyxScalarFunction *function, int big, int tuple, int ref)
{
    if (!!big + !!tuple + !!ref > 1)
    {
        return 0;
    }
    owner->request_tag = ref ? STYX_REF_REQUEST_TAG : tuple ? STYX_TUPLE_REQUEST_TAG : big ? STYX_FUNCTION_BIG_TAG : STYX_FUNCTION_REQUEST_TAG;
    function->request_tag = owner->request_tag;
    function->reply_tag = styxfunction_reply_tag(owner->request_tag);
    function->state = owner;
    if (ref)
    {
        function->ref_method = ref_dispatch;
    }
    else if (tuple)
    {
        function->tuple_method = tuple_dispatch;
    }
    else
    {
        function->method = styxscalarfunctiondispatch;
    }
    owner->request = styxunibusmailbox(function->request_tag);
    owner->reply = styxunibusmailbox(function->reply_tag);
    return owner->request && owner->reply;
}
