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
    StyxMux *mux = styxmuxcreatephy(phy, msize, timeout, limit);
    return mux ? styxclientstart(NULL, mux, msize, user, name) : NULL;
}
