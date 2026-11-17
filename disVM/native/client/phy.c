/* Copyright (c) 2026 bhgv
 * EN: Connect generic owned physical carriers to the existing native Styx transaction engine.
 * RU: Подключить generic owned physical carriers к существующему native Styx engine.
 */
#include "internal.h"


/* EN/RU: See client.h; shared native engine consumes PHY even on setup failure.
 * См. client.h; общий engine принимает PHY даже при setup failure. */
StyxClient *styxclientmountphy(RemotePhy *phy, unsigned msize, const char *user, const char *name,
    unsigned timeout, unsigned limit)
{
    PhyInfo info;
    int bidir = phyinfo(phy, &info) && (info.capabilities & PHY_BIDIR_SESSION);
    StyxMux *mux = styxmuxcreatephy(phy, msize, timeout, limit);
    StyxClient *client = mux ? styxclientstart(NULL, mux, msize, user, name) : NULL;
    if (client)
    {
        client->bidir = bidir;
    }
    return client;
}


/* EN/RU: See client.h; immutable native mode, no I/O or observation of a remote server.
 * См. client.h; immutable native mode, без I/O и наблюдения remote server. */
int styxclientbidir(StyxClient *client)
{
    return client && client->bidir;
}
