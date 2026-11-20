/* Copyright (c) 2026 bhgv
 * EN: Validate an ordinary Styx launch write before handing it to the owner.
 * RU: Проверить обычную запись Styx до передачи запуска владельцу.
 */
#include "launch_receive.h"
#include "flashvm/srecord.h"


/* EN/RU: See launch_receive.h; consume admission before any callback side effects.
 * См. launch_receive.h; поглотить выдачу до любых действий callback. */
int styxterminallaunchreceive(StyxLaunchReceiver *receiver, uint64_t offset,
    const void *bytes, size_t size)
{
    FVM_SRecord record;
    size_t consumed;
    if (!receiver || receiver->used || !receiver->start || !receiver->tag ||
        receiver->tag > FVM_SRECORD_U32 || offset || !bytes || !size || size > 32 ||
        fvm_srecord_read(bytes, size, 32, &record, &consumed) != FVM_S_OK ||
        consumed != size || record.tag != receiver->tag || record.size)
    {
        return 0;
    }
    receiver->used = 1;
    return receiver->start(receiver->state) == 1;
}
