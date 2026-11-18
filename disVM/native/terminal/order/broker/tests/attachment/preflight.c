/* Copyright (c) 2026 bhgv
 * EN: Native attachment preflight and initial-offer rollback on actual captured client/grant owners.
 * RU: Native preflight подключения и откат initial-offer с настоящими client/grant owners.
 */
#include "../../attach.h"
#include "cancel.h"
#include "flashvm/srecord.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned controls; /* Synchronous constructor invocations, not worker activity. */
static DVM_Cancel *cancel_after_control; /* Borrowed main-thread fixture source, never a worker callback. */
static int refuse_offer; /* Test-only refusal of next initial offer before it enters any wire queue. */


/* EN/RU: Original control constructor, caller-owned inputs remain borrowed.
 * Исходный конструктор control, inputs caller остаются borrowed.
 */
extern StyxBidirControl *__real_styxbidircontrol(StyxClient *, const DVM_TerminalChannels *, unsigned, unsigned);


/* EN/RU: Observe admission to actual lower setup; no fake handles or protocol implementation.
 * Наблюдать допуск к настоящему нижнему setup, без фиктивных handles/протокола.
 */
StyxBidirControl *__wrap_styxbidircontrol(StyxClient *client, const DVM_TerminalChannels *channels, unsigned limit, unsigned slots)
{
    controls++;
    StyxBidirControl *control = __real_styxbidircontrol(client, channels, limit, slots); /* Actual complete native owner. */
    if (cancel_after_control)
    {
        dvm_cancel_request(cancel_after_control);
    }
    return control;
}


/* EN/RU: Original native offer admission,1/0; may copy and number its own service request.
 * Исходный допуск предложения:1/0, может копировать/нумеровать свою служебную запись.
 */
extern int __real_styxorderedoffer(StyxOrdered *, unsigned long long, unsigned long);


/* EN/RU: Refuse once before offer publication to exercise complete staged-owner cleanup.
 * Отказать один раз до публикации offer для проверки очистки всех staged owners.
 */
int __wrap_styxorderedoffer(StyxOrdered *peer, unsigned long long operation, unsigned long lane)
{
    if (refuse_offer)
    {
        refuse_offer = 0;
        return 0;
    }
    return __real_styxorderedoffer(peer, operation, lane);
}


/* EN/RU: Mandatory test predicate, no optimized-away expressions.
 * Обязательная проверка теста, выражения не удаляются оптимизатором.
 */
static void require(int condition)
{
    if (!condition)
    {
        fputs("ordered attachment preflight/rollback failed\n", stderr);
        exit(2);
    }
}


/* EN/RU: Borrow healthy original session/channels/profile; reject malformed metadata before lower I/O and preserve owners.
 * Заимствовать исходные session/channels/profile; отвергать неверные metadata до lower I/O, сохраняя owners.
 */
void attachmentpreflight(StyxClient *client, const DVM_TerminalChannels *channels, const StyxOrderedOptions *valid)
{
    require(styxorderedoptionsvalid(valid) && !styxorderedoptionsvalid(NULL));
    for (unsigned fault = 0; fault < 10; fault++)
    {
        StyxOrderedOptions options = *valid; /* Mutable native preflight copy. */
        unsigned long tags[] = {7, 7}; /* Duplicate/oversized tag cases, private fixture values. */
        if (fault == 0)
        {
            options.struct_size--;
        }
        else if (fault == 1)
        {
            options.limit = 0;
        }
        else if (fault == 2)
        {
            options.limit = 65536;
        }
        else if (fault == 3)
        {
            options.window = 0;
        }
        else if (fault == 4)
        {
            options.window = 1ULL << 63;
        }
        else if (fault == 5)
        {
            options.lane = 0;
        }
        else if (fault == 6)
        {
            options.lane = FVM_SRECORD_U32 + 1UL;
        }
        else if (fault == 7)
        {
            options.tags = NULL;
        }
        else if (fault == 8)
        {
            options.tags = tags;
            options.count = 2;
        }
        else
        {
            tags[0] = FVM_SRECORD_U32;
            options.tags = tags;
            options.count = 1;
            options.limit = 1;
        }
        require(!styxorderedoptionsvalid(&options) && !styxbidirorderedchannels(client, channels, &options) && !controls);
    }
    StyxOrderedOptions empty = *valid; /* Native service-only profile needs no application grant. */
    empty.tags = NULL;
    empty.count = 0;
    require(styxorderedoptionsvalid(&empty));
    DVM_TerminalChannels invalid = *channels; /* Refuse unsafe path before lower control setup. */
    strcpy(invalid.receive, "../other");
    require(!styxbidirorderedchannels(client, &invalid, valid) && !controls);
    require(!styxbidirorderedlaunch(client, NULL, valid) && !controls);
    DVM_Cancel *cancel = dvm_cancel_create(); /* Own native pre-cancel source. */
    DVM_CancelScope scope = {0}; /* Main-thread scope, never propagated to broker workers. */
    require(cancel && dvm_cancel_enter(&scope, cancel));
    dvm_cancel_request(cancel);
    require(!styxbidirorderedchannels(client, channels, valid) && !controls);
    require(dvm_cancel_leave(&scope));
    dvm_cancel_release(cancel);
    refuse_offer = 1;
    require(!styxbidirorderedchannels(client, channels, valid) && controls == 1 && !refuse_offer && styxclientalive(client));
    cancel = dvm_cancel_create();
    require(cancel && dvm_cancel_enter(&scope, cancel));
    cancel_after_control = cancel;
    require(!styxbidirorderedchannels(client, channels, valid) && controls == 2 && styxclientalive(client));
    cancel_after_control = NULL;
    require(dvm_cancel_leave(&scope));
    dvm_cancel_release(cancel);
}
