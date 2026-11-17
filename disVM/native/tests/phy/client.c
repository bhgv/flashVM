/* Copyright (c) 2026 bhgv
 * EN: Real native Styx engine over a granted nonsocket PTY, including layered chunking and a final confirmed-prefix error.
 * RU: Настоящий native Styx engine через granted nonsocket PTY, включая layered chunking и final confirmed-prefix error.
 */
#include "client.h"
#include "drivers/posix/fd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "limited.h"


/* EN/RU: Explicit active invariant, without assert; diagnostic borrowed.
 * Явный active invariant без assert; diagnostic borrowed. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "PHY Styx: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Read one original 9P file over nonsocket carrier; optional pending watch is cancelled by host.
 * Читать original 9P file через nonsocket carrier; pending watch отменяется host. */
int main(int argc, char **argv)
{
    check(argc == 3, "inherited PTY descriptor/mode");
    int reject = !strcmp(argv[2], "reject");
    RemotePhy *phy = phyposixfd(atoi(argv[1]), PHY_STREAM|PHY_ORDERED|(reject ? 0 : PHY_RELIABLE));
    check(phy != NULL, "granted PTY driver");
    if (!reject)
    {
        phy = phyfixture(phy, !strcmp(argv[2], "prefix-error"));
        check(phy != NULL, "stacked carrier");
    }
    StyxClient *client = styxclientmountphy(phy, 256, "fixture", "", 3000, 4);
    if (reject)
    {
        check(!client, "unreliable carrier admitted to 9P");
        puts("PASS unreliable PHY rejected before 9P wire effects");
        return 0;
    }
    check(client && styxclientwatchable(client), "carrier-neutral native mount");
    StyxFid *fid = styxclientwalk(client, ".");
    check(fid && styxclientopen(fid, 0), "open remote file");
    char bytes[32];
    StyxTransfer result;
    styxclientread(fid, 0, bytes, sizeof(bytes), &result);
    check(result.success && result.confirmed && result.count == 6 && !memcmp(bytes, "serial", 6), "remote PTY bytes");
    if (!strcmp(argv[2], "prefix-error"))
    {
        check(!styxclientalive(client), "final carrier error was ignored");
    }
    styxclientclunk(fid);
    styxclientshutdown(client);
    styxclientfree(client);
    puts("PASS real native Styx transactions over nonsocket PTY PHY");
    return 0;
}
