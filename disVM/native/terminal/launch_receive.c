/* Copyright (c) 2026 bhgv
 * EN: Validate an ordinary Styx launch write before handing it to the owner.
 * RU: Проверить обычную запись Styx до передачи запуска владельцу.
 */
#include "launch_receive.h"
#include "flashvm/srecord.h"
#include "launch/ticket.h"


/* EN/RU: See launch_receive.h; consume admission before any callback side effects.
 * См. launch_receive.h; поглотить выдачу до любых действий callback. */
int styxterminallaunchreceive(StyxLaunchReceiver *receiver, uint64_t offset,
    const void *bytes, size_t size)
{
    FVM_SRecord record;
    size_t consumed;
    if (!receiver || receiver->used || (!receiver->start && !receiver->start_payload) || !receiver->tag ||
        receiver->tag > FVM_SRECORD_U32 || offset || !bytes || !size || size > DVM_LAUNCH_ARGUMENT_LIMIT + 32 ||
        fvm_srecord_read(bytes, size, DVM_LAUNCH_ARGUMENT_LIMIT + FVM_SRECORD_INTEGER_MAX,
            &record, &consumed) != FVM_S_OK || consumed != size || record.tag != receiver->tag ||
        record.size > DVM_LAUNCH_ARGUMENT_LIMIT ||
        (receiver->start_payload ? record.size > receiver->payload_limit : record.size != 0))
    {
        return 0;
    }
    receiver->used = 1;
    return (receiver->start_payload ? receiver->start_payload(receiver->state, record.payload, record.size) :
        receiver->start(receiver->state)) == 1;
}
