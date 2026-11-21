/* Copyright (c) 2026 bhgv
 * EN: Exchange a function call with the shared Dis owner service over local inboxes.
 * RU: Обмен вызовом с общим сервисом владельца Dis через локальные приёмники.
 */
#include <lib9.h>
#include "scalar_function.h"
#include "scalar_function_owner.h"
#include "record_write.h"
#include "scalar_verify.h"
#include "call_scope.h"
#include "cancel.h"
#include "flashvm/srecord.h"
#include <time.h>


/* EN: Exchange one complete request. A scoped call receives on its own mailbox.
 * Calls with a cancellation source wait until reply or revocation. Revocation
 * gives the owner a bounded interval to unwind and finish its local handoff.
 * RU: Передать полную запись. Вызов с контекстом принимает ответ в своём приёмнике.
 * При наличии токена ждать ответ либо отзыв. После отзыва дать владельцу
 * ограниченное время на разворачивание стека и завершение локального обмена. */
static int exchange(StyxScalarFunctionOwner *owner, unsigned char *record, size_t size, size_t *received, DVM_Provider *context)
{
    DVM_VfsHandle reader = {0};
    struct timespec start = {0};
    DVM_Provider *private_reply = context ? styxunibusloadmailbox() : NULL;
    int bound = !context || dvm_provider_set_call_reply(context, private_reply);
    DVM_Provider *reply = context ? private_reply : owner->reply;
    DVM_Cancel *cancel = dvm_provider_call_cancel(context);
    int bounded = !cancel;
    int opened = bound && !owner->retired && (!bounded || !clock_gettime(CLOCK_MONOTONIC, &start)) &&
        dvm_provider_open(reply, ".", "r", &reader) == FVM_OK;
    dvm_provider_release(private_reply);
    if (!opened)
    {
        dvm_cancel_release(cancel);
        return 0;
    }
    int ready = 0;
    unsigned char attached[STYX_FUNCTION_FRAME_CAPACITY + STYX_SCOPED_CALL_OVERHEAD];
    size_t length;
    if (context ? fvm_srecord_write(STYX_LOCAL_CONTEXT_TAG, record, size, attached, sizeof(attached), &length) == FVM_S_OK &&
            styxunibusmailboxcontext(owner->request, attached, length, context) :
        styxterminalrecordwrite(owner->request, record, size) == DVM_LAUNCH_DELIVERED)
    {
        struct timespec now;
        for (;;)
        {
            if (!bounded && dvm_cancel_requested(cancel))
            {
                if (clock_gettime(CLOCK_MONOTONIC, &start))
                {
                    break;
                }
                bounded = 1;
            }
            if (bounded && (clock_gettime(CLOCK_MONOTONIC, &now) ||
                (now.tv_sec - start.tv_sec) * INT64_C(1000) + (now.tv_nsec - start.tv_nsec) / 1000000 >= 3000))
            {
                break;
            }
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
    dvm_cancel_release(cancel);
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
        !exchange(owner, record, size, &received, NULL) ||
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
        exchange(owner, record, size, &received, NULL))
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
    StyxScalarFunctionOwner *owner = state;
    unsigned char record[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size, received;
    StyxScalarKind kind;
    int64_t code;
    if (styxtuplewrite(STYX_REF_REQUEST_TAG, 0, argument, record, sizeof(record), &size) &&
        exchange(owner, record, size, &received, self))
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
    return self ? -1 : retire(owner);
}


/* EN: Pass the argument list to the local owner's mailbox and validate its return.
 * RU: Передать список аргументов в приёмник владельца и проверить результат. */
static int list_dispatch(void *state, DVM_Provider *self, const StyxCallValues *arguments, StyxCallValues *value)
{
    StyxScalarFunctionOwner *owner = state;
    int dynamic = owner->request_tag == STYX_DYNAMIC_REQUEST_TAG;
    unsigned long reply_tag = styxfunction_reply_tag(owner->request_tag);
    const char *input_shape = dynamic ? "(bbbb)" : styxcall_list_args;
    const char *output_shape = dynamic ? "(bbbb)" : styxcall_list_result;
    StyxCallValues local = *arguments;
    if (dynamic)
    {
        local.count = STYX_DYNAMIC_CELLS;
    }
    unsigned char record[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size, received;
    StyxScalarKind kind;
    int64_t code;
    if (styxcallrecordwrite(owner->request_tag, input_shape, &local, record, sizeof(record), &size) &&
        exchange(owner, record, size, &received, self))
    {
        if (styxcallrecordread(reply_tag, output_shape, record, received, value))
        {
            if (dynamic)
            {
                value->count = owner->result_cells;
            }
            return 1;
        }
        if (styxscalarread(reply_tag, record, received, &kind, &code) && kind == STYX_SCALAR_ERROR)
        {
            return 0;
        }
    }
    return self ? -1 : retire(owner);
}


/* EN: Invoke the shared dispatcher without a borrowed context for legacy profiles.
 * RU: Вызвать общий диспетчер без заимствованного контекста для прежнего профиля. */
static int plain_list_dispatch(void *state, const StyxCallValues *arguments, StyxCallValues *value)
{
    return list_dispatch(state, NULL, arguments, value);
}


/* EN: Exchange the numbered snapshots with the linked Dis service. Reject a
 * changed identity map as an ambiguous owner response and retire its mailbox.
 * RU: Обменять снимки со связанным сервисом Dis. При изменении таблицы
 * идентичности закрыть приёмник, поскольку ответ владельца неоднозначен. */
static int ref_pair_dispatch(void *state, DVM_Provider *self, StyxRefPair *value)
{
    StyxScalarFunctionOwner *owner = state;
    unsigned char record[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size, received;
    StyxRefPair result;
    StyxScalarKind kind;
    int64_t code;
    if (styxrefpairwrite(STYX_REF_PAIR_REQUEST_TAG, value, record, sizeof(record), &size) &&
        exchange(owner, record, size, &received, self))
    {
        if (styxrefpairread(STYX_REF_PAIR_REPLY_TAG, record, received, &result) && result.count == value->count)
        {
            *value = result;
            return 1;
        }
        if (styxscalarread(STYX_REF_PAIR_REPLY_TAG, record, received, &kind, &code) && kind == STYX_SCALAR_ERROR)
        {
            return 0;
        }
    }
    return self ? -1 : retire(owner);
}


/* EN: Dispatch a plain pair without a caller context.
 * RU: Выполнить обычный парный вызов без контекста клиента. */
static int plain_ref_pair_dispatch(void *state, StyxRefPair *value)
{
    return ref_pair_dispatch(state, NULL, value);
}


/* EN/RU: See scalar_function_owner.h. / См. scalar_function_owner.h. */
int styxscalarownerconfigure(StyxScalarFunctionOwner *owner, StyxScalarFunction *function, int big, int tuple, int ref, int call, int ref_pair)
{
    if (!!big + !!tuple + !!ref + !!call + !!ref_pair > 1)
    {
        return 0;
    }
    unsigned long tag = ref_pair ? STYX_REF_PAIR_REQUEST_TAG : call ? STYX_CALL_LIST_REQUEST_TAG :
        ref ? STYX_REF_REQUEST_TAG : tuple ? STYX_TUPLE_REQUEST_TAG :
        big ? STYX_FUNCTION_BIG_TAG : STYX_FUNCTION_REQUEST_TAG;
    if (!styxscalarownerbind(owner, function, tag))
    {
        return 0;
    }
    owner->request = styxunibusmailbox(function->request_tag);
    owner->reply = styxunibusmailbox(function->reply_tag);
    return owner->request && owner->reply;
}


/* EN/RU: See scalar_function_owner.h. / См. scalar_function_owner.h. */
int styxscalarownerbind(StyxScalarFunctionOwner *owner, StyxScalarFunction *function, unsigned long tag)
{
    function->reply_tag = styxfunction_reply_tag(tag);
    if (!function->reply_tag)
    {
        return 0;
    }
    owner->request_tag = function->request_tag = tag;
    function->state = owner;
    switch (tag)
    {
    case STYX_DYNAMIC_REQUEST_TAG:
    {
        size_t arguments;
        if (!styxshapecells(&owner->shape, &arguments, &owner->result_cells))
        {
            return 0;
        }
        function->dynamic_shape = &owner->shape;
        if (function->program_root)
        {
            function->scoped_call_method = list_dispatch;
        }
        else
        {
            function->call_method = plain_list_dispatch;
        }
        break;
    }
    case STYX_REF_PAIR_REQUEST_TAG:
        if (function->program_root)
        {
            function->scoped_ref_pair_method = ref_pair_dispatch;
        }
        else
        {
            function->ref_pair_method = plain_ref_pair_dispatch;
        }
        break;
    case STYX_CALL_LIST_REQUEST_TAG:
        function->call_method = plain_list_dispatch;
        break;
    case STYX_REF_REQUEST_TAG:
        function->ref_method = ref_dispatch;
        break;
    case STYX_TUPLE_REQUEST_TAG:
        function->tuple_method = tuple_dispatch;
        break;
    case STYX_FUNCTION_BIG_TAG:
    case STYX_FUNCTION_REQUEST_TAG:
        function->method = styxscalarfunctiondispatch;
        break;
    default:
        return 0;
    }
    return 1;
}
