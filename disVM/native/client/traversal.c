/* Copyright (c) 2026 bhgv
 * EN: Explicit original-Styx-over-S-record history profile; no peer guessing or change to raw 9P constructors.
 * RU: Явный original-Styx-over-S-record history profile; без угадывания peer и изменения raw 9P constructors.
 */
#include "mux_internal.h"
#include "packet.h"


/* EN/RU: See client.h; configure the profile before native owner publication/Tversion, consuming PHY always.
 * См. client.h; настроить profile до native owner publication/Tversion; PHY принять всегда.
 */
StyxClient *styxclientwalkmountphy(RemotePhy *phy, unsigned msize, const char *user, const char *name,
    unsigned timeout, unsigned limit, unsigned long tag)
{
    PhyInfo info;
    int bidir = phyinfo(phy, &info) && (info.capabilities & PHY_BIDIR_SESSION);
    StyxMux *mux = tag && tag <= 0xffffffffUL ? styxmuxcreatewalkphy(phy, msize, timeout, limit, tag) : NULL;
    if (!mux)
    {
        if (!tag || tag > 0xffffffffUL)
        {
            phyfree(phy);
        }
        return NULL;
    }
    StyxClient *client = styxclientstart(NULL, mux, msize, user, name);
    if (client)
    {
        client->bidir = bidir;
    }
    return client;
}


/* EN/RU: See client.h; copy only an acknowledged fixed root description, never an authority inferred from an ID.
 * См. client.h; копировать лишь acknowledged fixed root description, не authority из ID.
 */
int styxclientwalktarget(StyxClient *client, WalkEntity *root)
{
    if (!client || !client->mux || !root)
    {
        return 0;
    }
    StyxMux *mux = client->mux;
    pthread_mutex_lock(&mux->lock);
    int valid = mux->walk_tag && mux->walk_bound && !mux->failed;
    if (valid)
    {
        *root = mux->walk_root;
    }
    pthread_mutex_unlock(&mux->lock);
    return valid;
}


/* EN/RU: Under mux lock, validate the envelope then restore unchanged raw frame for original codec/response-tag processing.
 * Под mux lock проверить envelope, восстановить raw frame для исходного codec/tag processing;1/0.
 */
int styxmuxwalkreply(StyxMux *mux, unsigned *cycle)
{
    WalkPacket packet;
    if (!walk_packet_read(mux->receive, mux->want, &packet) || packet.tag != mux->walk_tag ||
        (packet.kind & ~WALK_PACKET_CYCLE) != WALK_PACKET_REPLY || packet.frame_size > mux->capacity ||
        (mux->walk_bound && memcmp(&mux->walk_root, &packet.root, sizeof(packet.root))))
    {
        return 0;
    }
    mux->walk_root = packet.root;
    mux->walk_bound = 1;
    *cycle = (packet.kind & WALK_PACKET_CYCLE) != 0;
    memmove(mux->receive, packet.frame, packet.frame_size);
    mux->want = (unsigned)packet.frame_size;
    return 1;
}


/* EN/RU: See client.h; comparison identity never replaces the fid used by the actual operation.
 * См. client.h; comparison identity не заменяет fid, используемый actual operation.
 */
int styxclientwalkprobe(StyxClient *client, StyxFid *base, const char *relative, unsigned operation, const char *detail)
{
    WalkEntity root;
    if (!client || !relative || !styxclientwalktarget(client, &root))
    {
        return WALK_OK;
    }
    const char *prefix = base ? base->walk_path : ".";
    size_t first = strcmp(prefix, ".") ? strlen(prefix) : 0;
    size_t tail = strcmp(relative, ".") ? strlen(relative) : 0;
    char path[STYX_NAME_MAX];
    if (first + tail + 2 > sizeof(path))
    {
        return WALK_INVALID;
    }
    memcpy(path, prefix, first);
    if (first && tail)
    {
        path[first++] = '/';
    }
    memcpy(path + first, relative, tail);
    path[first + tail] = 0;
    if (!first && !tail)
    {
        strcpy(path, ".");
    }
    WalkScope scope = {0};
    uvlong node = base && (operation == WALK_FSTAT || operation == WALK_LIST) ? base->qid.path : 0;
    int result = walk_enter(&scope, &root, (unsigned long)(node >> 32), (unsigned long)(node & 0xffffffffULL),
        operation, path, detail);
    if (result == WALK_OK && !walk_leave(&scope))
    {
        abort();
    }
    return result;
}
