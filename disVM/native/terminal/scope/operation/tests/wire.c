/* Copyright (c) 2026 bhgv
 * EN: Independent original-Styx peer oracle for fixture-only proposal/ACK/NACK bodies and activation barriers.
 * RU: Independent original-Styx peer oracle для fixture-only proposal/ACK/NACK bodies и activation barriers.
 */
#include "flashvm/srecord.h"
#include <stdlib.h>
#include <string.h>
static unsigned stage; /* Sole original-server-owner exchange phase. */
static int rx_swapped; /* Peer changes outbound RX envelope only after actual positive confirmation. */
static int group_pending; /* Independent group proposal remains pending while root proposal also waits. */
static int group_done; /* Group local tag33 activated independently of root local tag33. */
static unsigned old;   /* Old TX frame witnessed before proposal; original stream ordering proves barrier. */


/* EN/RU: Original server-owner output helpers, copy before return, no native client shortcuts.
 * Исходные server-owner output helpers, copy до return, без native client shortcuts.
 */
extern void scopeoperationoffer(unsigned long, const void *, size_t);


/* EN/RU: Original nested client output helper, copy before return on the server owner.
 * Исходный nested client output helper, copy до return на server owner.
 */
extern void scopeoperationclient(unsigned long, unsigned long, const void *, size_t);


/* EN/RU: Compare exact borrowed fixture body, binary extent preserved.
 * Сравнить exact borrowed fixture body, сохраняя binary extent.
 */
static int is(const FVM_SRecord *record, const char *value)
{
    return record->size == strlen(value) && !memcmp(record->payload, value, record->size);
}


/* EN/RU: Emit one native root service response via original S-record codec.
 * Выдать native root service response через original S-record codec.
 */
static void service(const char *value)
{
    scopeoperationoffer(0, value, strlen(value));
}


/* EN/RU: Decode a client-local7 frame on an independently checked outer TX tag.
 * Декодировать client-local7 frame в independently checked outer TX tag.
 */
static int data(const FVM_SRecord *outer)
{
    FVM_SRecord inner; /* Borrowed actual nested body view. */
    size_t used; /* Complete original frame extent. */
    if (fvm_srecord_read(outer->payload, outer->size, 4096, &inner, &used) != FVM_S_OK || used != outer->size)
    {
        return -1;
    }
    if (outer->tag == 50)
    {
        FVM_SRecord local; /* Another tag namespace inside root envelope50. */
        size_t nested;
        if (fvm_srecord_read(inner.payload, inner.size, 4096, &local, &nested) != FVM_S_OK || nested != inner.size || local.tag != 7)
        {
            return -1;
        }
        if (inner.tag == 52 && group_pending && is(&local, "D-live"))
        {
            service("D-seen");
        }
        else if (inner.tag == 33 && group_done && is(&local, "C-new"))
        {
            service("C-seen");
        }
        else
        {
            return -1;
        }
        return 1;
    }
    if (inner.tag != 7)
    {
        return -1;
    }
    if (outer->tag == 31 && !stage && is(&inner, "old"))
    {
        old++;
        service("old-seen");
    }
    else if (outer->tag == 32 && stage == 1 && is(&inner, "B-live"))
    {
        service("B-seen");
    }
    else if (outer->tag == 32 && stage == 3 && is(&inner, "B-during-ack"))
    {
        service("B-during-ack-seen");
    }
    else if (outer->tag == 32 && stage == 7 && is(&inner, "B-live"))
    {
        service("B-uncertain-seen");
    }
    else if (outer->tag == 33 && stage == 2 && is(&inner, "new"))
    {
        service("new-seen");
    }
    else if (outer->tag == 33 && stage == 3 && is(&inner, "after-nack"))
    {
        service("nack-seen");
    }
    else
    {
        return -1;
    }
    return 1;
}


/* EN/RU: Parse fixture-only service strings, never a production protocol/schema declaration.
 * Разобрать fixture-only service strings, не production protocol/schema declaration.
 */
int scopeoperationcommand(const FVM_SRecord *record)
{
    if (record->tag == 31 || record->tag == 32 || record->tag == 33 || record->tag == 50)
    {
        return data(record);
    }
    if (record->tag)
    {
        return 0;
    }
    if (is(record, "proposal:1") && !stage && old == 1)
    {
        stage = 1;
        scopeoperationclient(11, 0, "ack:1", 5); /* A local client cannot confirm a root operation. */
        scopeoperationclient(11, 7, "rx-during-wait", 14);
        service("pending:1");
    }
    else if (is(record, "proposal:group") && stage == 1 && !group_pending && !group_done)
    {
        group_pending = 1;
        service("group-pending");
    }
    else if (is(record, "release:group") && stage == 1 && group_pending)
    {
        group_pending = 0;
        group_done = 1;
        service("ack-group:1");
        service("group-confirmed");
    }
    else if (is(record, "release:1") && stage == 1)
    {
        stage = 2;
        service("ack:1");
        service("confirmed:1");
    }
    else if (is(record, "proposal:2") && stage == 2)
    {
        stage = 3;
        service("pending:2");
    }
    else if (is(record, "release:2") && stage == 3)
    {
        service("nack:2");
        service("confirmed:2");
    }
    else if (is(record, "change-rx") && stage == 3)
    {
        scopeoperationclient(11, 7, "before-rx", 9);
        service("request-rx:4");
        service("probe-during-ack"); /* Broker must process this while its service sender is blocked. */
    }
    else if (is(record, "rx-ok:4") && stage == 3)
    {
        stage = 4;
        rx_swapped = 1;
        scopeoperationclient(12, 7, "after-rx", 8); /* Only after receiving actual ACK, never by local test shortcut. */
        service("rx-accepted:4");
    }
    else if (is(record, "rx-no:4") && stage == 3)
    {
        stage = 4;
        scopeoperationclient(11, 7, "after-rejected-rx", 17);
        service("rx-rejected:4");
    }
    else if (is(record, "bad-rx") && stage == 4)
    {
        stage = 5;
        service("request-rx:5");
    }
    else if (is(record, "rx-no:5") && stage == 5)
    {
        stage = 6;
        scopeoperationclient(rx_swapped ? 12 : 11, 7, "unchanged-rx", 12);
        service("rx-rejected:5");
    }
    else if (is(record, "proposal:3") && stage == 6)
    {
        stage = 7;
        scopeoperationclient(rx_swapped ? 12 : 11, 7, "rx-while-uncertain", 18);
        service("pending:3"); /* Intentionally no ACK/NACK for uncertain-operation tests. */
    }
    else if (is(record, "malformed") && stage == 7)
    {
        service("ack:invalid");
    }
    else if (is(record, "replace") && stage == 7)
    {
        service("replace-handler");
    }
    else if (is(record, "replacement-ready") && stage == 7)
    {
        service("replacement-observed");
    }
    else
    {
        return -1;
    }
    return 1;
}
