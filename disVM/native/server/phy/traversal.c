/* Copyright (c) 2026 bhgv
 * EN: Explicit S-record history server admission and scoped restoration before original libstyx dispatch.
 * RU: Явный S-record history server admission и scoped restoration до исходного libstyx dispatch.
 */
#include "internal.h"
#include "packet.h"


/* EN/RU: Validate native granted-root description; IDs describe a projection and never authenticate the peer.
 * Проверить native granted-root description; IDs описывают projection, не аутентифицируют peer.
 */
static int root_valid(const WalkEntity *root)
{
    unsigned long domain = 0;
    unsigned long object = 0;
    if (!root)
    {
        return 0;
    }
    for (unsigned i = 0; i < 4; i++)
    {
        if (root->domain[i] > 0xffffffffUL || root->object[i] > 0xffffffffUL)
        {
            return 0;
        }
        domain |= root->domain[i];
        object |= root->object[i];
    }
    return domain && object;
}


/* EN/RU: See server.h; explicit mode cannot fall back to raw 9P after malformed/unsupported control traffic.
 * См. server.h; explicit mode не делает fallback к raw 9P при malformed/unsupported control traffic.
 */
char *styxphywalkinit(Styxserver *server, Styxops *ops, RemotePhy *phy,
    int permissions, int needfile, unsigned queued_frames, const WalkEntity *root)
{
    if (!root_valid(root))
    {
        phyfree(phy);
        return "invalid traversal root identity";
    }
    return styxphyinitprofile(server, ops, phy, permissions, needfile, queued_frames, root);
}


/* EN/RU: Restore copied request-local state before raw libstyx sees the unchanged frame;1/0.
 * scope belongs to caller through dispatch/leave; receive buffer may now be reused safely.
 * Восстановить copied request-local state до raw libstyx frame;1/0. Scope caller до dispatch/leave.
 */
int styxphywalkenter(StyxPhyWake *context, WalkScope *scope)
{
    WalkPacket packet;
    if (!walk_packet_read(context->receive, context->used, &packet) || packet.kind != WALK_PACKET_REQUEST ||
        packet.frame_size > MSGMAX || (context->walk_tag && context->walk_tag != packet.tag) ||
        walk_import(scope, packet.history, packet.history_size) != WALK_OK)
    {
        return 0;
    }
    context->walk_tag = packet.tag;
    memmove(context->receive, packet.frame, packet.frame_size);
    context->used = context->wanted = (unsigned)packet.frame_size;
    return 1;
}


/* EN/RU: Original API selects unchanged raw protocol; no hidden peer sniffing or control record interpretation.
 * Исходный API выбирает raw protocol; без hidden peer sniffing/control record interpretation.
 */
char *styxphyinit(Styxserver *server, Styxops *ops, RemotePhy *phy,
    int permissions, int needfile, unsigned queued_frames)
{
    return styxphyinitprofile(server, ops, phy, permissions, needfile, queued_frames, NULL);
}


/* EN/RU: See server.h; only the currently owned callback may attach an explicit cycle outcome to its own reply.
 * См. server.h; только current owner callback может отметить cycle outcome собственного reply.
 */
char *styxphycycleerror(Styxserver *server)
{
    StyxPhyWake *context = server ? server->priv : NULL;
    if (context && context == styxphyowner && context->processing)
    {
        context->walk_cycle = 1;
    }
    return "namespace traversal cycle";
}


/* EN/RU: See internal.h; pending scalar candidate is retired by normal response tracking, original fids remain untouched.
 * См. internal.h; normal response tracking освобождает pending scalar candidate, original fids не меняются.
 */
int styxphywalkreject(Styxserver *server)
{
    StyxPhyWake *context = server->priv;
    Fcall reply = {0};
    unsigned char bytes[128];
    reply.type = Rerror;
    reply.tag = context->walk_request.tag;
    reply.ename = styxphycycleerror(server);
    unsigned length = convS2M(&reply, bytes, sizeof(bytes));
    int accepted = length && styxsend(server, 2, (char *)bytes, (int)length, 0) == (int)length;
    context->used = 0;
    context->wanted = 4;
    context->complete = 0;
    return accepted;
}
