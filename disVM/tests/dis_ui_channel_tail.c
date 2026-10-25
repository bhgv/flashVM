/* Appended to the unchanged upstream channel fixture; scheduler and allocator are fixture boundaries. */
#include "disvm/dis_ui_channel.h"
#include "disvm/ui_pump.h"

Type Tptr = {1};  /* Pointer descriptor used by the real Dis mover/freeptrs path. */
Type Tbyte = {1}; /* One-byte nonpointer descriptor for the copied wire array. */


/* Cross a production Dis byte-array channel and the actual FlashVM queue in one test.
 * No parameters. Returns zero on balanced ownership and correct values; one with diagnostics. */
int main(void)
{
    TestChannel box;
    Channel *channel;
    Array *frame;
    DVM_UIBridge *bridge;
    DVM_UIMessage message;
    DVM_UIMessage received;
    REG saved;
    Prog peer;
    int before;

    scalar.size = sizeof(WORD);
    pointer.size = sizeof(void *);
    pointer.np = 1;
    pointer.map[0] = 0x80;
    Tptr = pointer;
    Tbyte.size = 1;
    memset(&message, 0, sizeof(message));
    message.kind = DVM_UI_REPLY;
    message.id = 29;
    message.first = -17;
    message.size = 5;
    memmove(message.text, "A\0\xd0\xafZ", 5);
    openchannel(&box, 0, 1);
    before = live;
    memset(&R, 0x35, sizeof(R));
    saved = R;
    if (dvm_ui_dis_offer(&box.c, &message) != FVM_NOT_FOUND || box.c.buf != H || live != before || Tbyte.ref != 1 ||
        Tarray.ref != 1 || memcmp(&R, &saved, sizeof(R)))
    {
        fprintf(stderr, "unready Dis rendezvous changed buffer, roots or registers\n");
        return 1;
    }
    closechannel(&box.c);
    openchannel(&box, 0, 1);
    memset(&peer, 0, sizeof(peer));
    frame = H;
    peer.state = Precv;
    peer.ptr = &frame;
    cqadd(&box.c.recv, &peer);
    if (dvm_ui_dis_offer(&box.c, &message) != FVM_OK || frame == H || peer.state != Pready || frame->t != &Tbyte ||
        memcmp(&R, &saved, sizeof(R)))
    {
        fprintf(stderr, "real Dis rendezvous did not transfer/wake/restore registers\n");
        return 1;
    }
    destroy(frame);
    closechannel(&box.c);
    openchannel(&box, 1, 1);
    if (dvm_ui_dis_offer(&box.c, &message) != FVM_OK || memcmp(&R, &saved, sizeof(R)))
    {
        fprintf(stderr, "production Dis buffered delivery failed\n");
        return 1;
    }
    channel = &box.c;
    frame = H;
    R.s = &channel;
    R.d = &frame;
    irecv();
    if (frame == H || frame->t != &Tbyte || dvm_ui_create(0, &bridge) != FVM_OK ||
        dvm_ui_dis_export(bridge, frame) != FVM_OK || dvm_ui_receive(bridge, DVM_UI_UI, &received, 0) != FVM_OK ||
        received.id != 29 || received.first != -17 || received.size != 5 || memcmp(received.text, message.text, 5))
    {
        fprintf(stderr, "Dis byte-array receive to FlashVM export failed\n");
        return 1;
    }
    destroy(frame);
    closechannel(&box.c);
    dvm_ui_close(bridge);
    dvm_ui_destroy(bridge);
    if (live || nodes || Tbyte.ref != 1 || Tptr.ref != 1 || Tarray.ref != 1)
    {
        fprintf(stderr, "Dis bridge leaked a heap reference or queue node\n");
        return 1;
    }
    {
        TestChannel requests;
        TestChannel replies;
        DVM_UIDisEndpoint endpoint;
        DVM_UIPump *pump;
        DVM_UIPumpState state;
        DVM_UIMessage pending;
        unsigned int index;
        openchannel(&requests, 0, 1);
        openchannel(&replies, 1, 1);
        endpoint.requests = &requests.c;
        endpoint.replies = &replies.c;
        if (dvm_ui_create(0, &bridge) != FVM_OK ||
            dvm_ui_pump_create(bridge, dvm_ui_dis_pump_offer, dvm_ui_dis_pump_take, &endpoint, &pump) != FVM_OK)
        {
            fprintf(stderr, "Dis owner pump initialization failed\n");
            return 1;
        }
        message.kind = DVM_UI_REQUEST;
        message.id = 73;
        if (dvm_ui_send(bridge, DVM_UI_UI, &message, 0) != FVM_OK || dvm_ui_pump_step(pump, 1, &state) != FVM_OK ||
            !state.pending_service || state.to_service != 0 || dvm_ui_pump_pending(pump, 1, &pending) != FVM_OK ||
            pending.id != 73)
        {
            fprintf(stderr, "Dis owner pump lost backpressured ingress\n");
            return 1;
        }
        memset(&peer, 0, sizeof(peer));
        frame = H;
        peer.state = Precv;
        peer.ptr = &frame;
        cqadd(&requests.c.recv, &peer);
        if (dvm_ui_pump_step(pump, 1, &state) != FVM_OK || state.pending_service || state.to_service != 1 ||
            frame == H || peer.state != Pready)
        {
            fprintf(stderr, "Dis owner pump failed retained rendezvous delivery\n");
            return 1;
        }
        destroy(frame);
        message.kind = DVM_UI_REPLY;
        if (dvm_ui_dis_offer(&replies.c, &message) != FVM_OK || dvm_ui_pump_step(pump, 1, &state) != FVM_OK ||
            state.to_ui != 1 || dvm_ui_receive(bridge, DVM_UI_UI, &received, 0) != FVM_OK || received.id != 73)
        {
            fprintf(stderr, "production Dis irecv-to-UI pump routing failed\n");
            return 1;
        }
        for (index = 0; index < DVM_UI_PENDING_MAX; index++)
        {
            message.id = 100 + index;
            if (dvm_ui_send(bridge, DVM_UI_DIS, &message, 0) != FVM_OK)
            {
                fprintf(stderr, "UI pressure preparation failed\n");
                return 1;
            }
        }
        message.id = 73;
        if (dvm_ui_dis_offer(&replies.c, &message) != FVM_OK || dvm_ui_pump_step(pump, 1, &state) != FVM_OK ||
            !state.pending_ui || dvm_ui_pump_pending(pump, 0, &pending) != FVM_OK || pending.id != 73 ||
            dvm_ui_receive(bridge, DVM_UI_UI, &received, 0) != FVM_OK || received.id != 100 ||
            dvm_ui_pump_step(pump, 1, &state) != FVM_OK || state.pending_ui || state.to_ui != 2)
        {
            fprintf(stderr, "Dis owner pump lost/reordered backpressured output\n");
            return 1;
        }
        for (index = 1; index < DVM_UI_PENDING_MAX; index++)
        {
            if (dvm_ui_receive(bridge, DVM_UI_UI, &received, 0) != FVM_OK || received.id != 100 + index)
            {
                fprintf(stderr, "owner output overtook an earlier queued reply\n");
                return 1;
            }
        }
        if (dvm_ui_receive(bridge, DVM_UI_UI, &received, 0) != FVM_OK || received.id != 73)
        {
            fprintf(stderr, "retained Dis reply did not reach the UI\n");
            return 1;
        }
        /* Allocation failure must stop routing without dropping the retained UI request. */
        message.kind = DVM_UI_REQUEST;
        message.id = 74;
        if (dvm_ui_send(bridge, DVM_UI_UI, &message, 0) != FVM_OK)
        {
            fprintf(stderr, "allocation-fault request enqueue failed\n");
            return 1;
        }
        saved = R;
        fail_after = 0;
        if (dvm_ui_pump_step(pump, 1, &state) != FVM_ERROR || !state.stopped || !state.pending_service ||
            memcmp(&R, &saved, sizeof(R)))
        {
            fprintf(stderr, "Dis error escaped owner boundary or discarded pending request\n");
            return 1;
        }
        fail_after = -1;
        if (dvm_ui_pump_pending(pump, 1, &pending) != FVM_OK || pending.id != 74 || dvm_ui_pump_stop(pump) != FVM_OK ||
            dvm_ui_pump_destroy(pump) != FVM_OK)
        {
            fprintf(stderr, "Dis failed owner pump recovery/teardown failed\n");
            return 1;
        }
        dvm_ui_destroy(bridge);
        closechannel(&requests.c);
        closechannel(&replies.c);
        if (live || nodes || worker.nerr || Tbyte.ref != 1 || Tptr.ref != 1 || Tarray.ref != 1)
        {
            fprintf(stderr, "owner pump fault path leaked Dis references/queues/error frames\n");
            return 1;
        }
    }
    puts("PASS production Dis array/mover/rendezvous rejection and real FlashVM export");
    return 0;
}
