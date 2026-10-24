/* Appended to the unchanged upstream channel fixture; scheduler and allocator are fixture boundaries. */
#include "disvm/dis_ui_channel.h"

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
    puts("PASS production Dis array/mover/rendezvous rejection and real FlashVM export");
    return 0;
}
