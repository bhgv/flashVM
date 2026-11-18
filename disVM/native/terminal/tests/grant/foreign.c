/* Copyright (c) 2026 bhgv
 * EN: A second real original-9P client/server pair tests foreign grant identity without forged native pointers.
 * RU: Вторая real original-9P client/server пара проверяет чужой grant без поддельных native pointers.
 */
#include <sys/socket.h>
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "client.h"
#include "server.h"
#include "drivers/posix/fd.h"
#include <pthread.h>
static Styxserver server; /* Dedicated original owner through join. */
static pthread_t worker; /* Native server owner, joined after foreign client/root retirement. */
static Styxops operations; /* Original server borrows this table through final join, never a returned stack. */


/* EN/RU: Original server owns all callbacks and consumes its PHY on exit; no guest or shared main session.
 * Original server владеет callbacks и поглощает PHY при exit, без guest либо main session.
 */
static void *serve(void *unused)
{
    USED(unused);
    while (!styxwait(&server) && !styxprocess(&server))
    {
    }
    styxend(&server);
    return nil;
}


/* EN/RU: Create an owned independent native client/NULL; the local socketpair is fixture-only PHY, not P9MX.
 * Свой independent native client/NULL; local socketpair — только fixture PHY, не P9MX.
 */
StyxClient *terminalgrantforeign(void)
{
    int pair[2]; /* Independent connected fixture carrier, no relation to main peer descriptors. */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair))
    {
        exit(3);
    }
    if (styxphyinit(&server, &operations, phyposixfd(pair[0], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 0555, 1, 2) ||
        pthread_create(&worker, nil, serve, nil))
    {
        exit(3);
    }
    return styxclientmountphy(phyposixfd(pair[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 256, "fixture", "", 3000, 2);
}


/* EN/RU: Consume foreign owner after its grant refs were released; join only the independent fixture server.
 * Поглотить foreign owner после release grant refs; join только independent fixture server.
 */
void terminalgrantforeignfree(StyxClient *client)
{
    styxclientfree(client);
    if (pthread_join(worker, nil))
    {
        exit(3);
    }
}
