/* Copyright (c) 2026 bhgv
 * EN: Actual native client reads a remote file through retained fids, without VM or exported host
 * paths.
 * RU: Настоящий нативный клиент читает удалённый файл через удерживаемые fid, без VM и экспорта
 * host-путей.
 * standalone-reviewed: authored teaching contracts; executable tokens unchanged.
 */
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>


/* EN: Read argv[2] from a loopback 9P server at argv[1] (argc must be 3). Own the socket until
 * mountfd consumes it on either outcome; then own client/fid until clunk, shutdown and free. Read
 * confirmed prefixes only, advance offset by count and stop at confirmed zero. Return 1 for
 * arguments/socket, 2 for mount, 3 for walk/open/read/short-fwrite failure, 0 for EOF. Synchronous CLI;
 * no VM or shared client access.
 * RU: Прочитать argv[2] с loopback 9P сервера argv[1] (argc=3). Владеть socket до mountfd,
 * поглощающего его при любом исходе; затем client/fid до clunk, shutdown/free. Выводить только
 * confirmed prefixes, двигать offset по count, завершать при confirmed zero. Коды: 1
 * arguments/socket, 2 mount, 3 walk/open/read/short-fwrite, 0 EOF. Синхронный CLI, без VM и общего client
 * access.
 */
int main(int argc, char **argv)
{
    char *end;
    unsigned long port = argc == 3 ? strtoul(argv[1], &end, 10) : 0;
    if (!port || port > 65535 || *end)
    {
        return 1;
    }
    int descriptor = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    if (descriptor < 0)
    {
        return 1;
    }
    if (connect(descriptor, (struct sockaddr *)&address, sizeof(address)))
    {
        close(descriptor);
        return 1;
    }
    StyxClient *client = styxclientmountfd(descriptor, 8192, "demo", "", 5000, 8);
    if (!client)
    {
        return 2;
    }
    StyxFid *fid = styxclientwalk(client, argv[2]);
    int valid = fid && styxclientopen(fid, 0);
    unsigned long long offset = 0;
    while (valid)
    {
        unsigned char bytes[1024];
        StyxTransfer result;
        styxclientread(fid, offset, bytes, sizeof(bytes), &result);
        if (!result.success || !result.confirmed || result.uncertain)
        {
            valid = 0;
            break;
        }
        if (!result.count)
        {
            break;
        }
        if (fwrite(bytes, 1, result.count, stdout) != result.count)
        {
            valid = 0;
            break;
        }
        offset += result.count;
    }
    styxclientclunk(fid);
    styxclientshutdown(client);
    styxclientfree(client);
    return valid ? 0 : 3;
}
