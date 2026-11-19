/* Copyright (c) 2026 bhgv
 * EN: Real original 9P server and native client over a descriptor-free memory PHY.
 * RU: Настоящий исходный 9P server и native client через memory PHY без дескрипторов.
 * standalone-reviewed: transport grants move once; all native owners retire before stats/storage.
 */
#include "demo.h" /* Private startup state; only server thread owns the original server object. */
#include "client.h" /* Native client uses original Inferno codecs, not a demonstration wire decoder. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN: Borrow client through this call. Open/read/stat/clunk a real remote file;
 * reject write-open, require high64-offset EOF, then verify a fresh read still works.
 * Return 1/0; consume every fid before returning, no automatic retry of failed effects.
 * RU: Заимствовать client на время вызова. Реально открыть/read/stat/clunk файл;
 * отклонить write-open, проверить high64 EOF и последующее обычное чтение.
 * Вернуть 1/0; поглотить все fids до возврата, без автоматического retry failed effects.
 */
static int use_file(StyxClient *client)
{
    StyxFid *denied = styxclientwalk(client, "status"); /* Independent fid: denial is not permission to reuse an uncertain open. */
    int good = denied && !styxclientopen(denied, 1) && styxclientalive(client);
    styxclientclunk(denied);
    StyxFid *fid = styxclientwalk(client, "status");
    if (!fid)
    {
        return 0;
    }
    good = good && styxclientopen(fid, 0);
    StyxMeta metadata;
    good = good && styxclientstat(fid, &metadata) && !strcmp(metadata.name, "status") && metadata.length == 21;
    unsigned char bytes[64]; /* Read acknowledgement count, not the transport's per-callback chunk size. */
    StyxTransfer result;
    if (good)
    {
        styxclientread(fid, 0, bytes, sizeof(bytes), &result);
        good = result.success && result.count == 21 && !memcmp(bytes, "memory-only provider\n", 21);
    }
    if (good)
    {
        styxclientread(fid, 0x7fffffffffffffffULL, bytes, sizeof(bytes), &result);
        good = result.success && !result.count; /* Full signed64 cursor crosses the wire unchanged. */
    }
    if (good)
    {
        styxclientread(fid, 7, bytes, sizeof(bytes), &result);
        good = result.success && result.count == 14 && !memcmp(bytes, "only provider\n", 14);
    }
    styxclientclunk(fid); /* Consume before dropping the client's owning connection reference. */
    return good;
}


/* EN: No arguments. Own endpoint pair, startup synchronization and server thread.
 * Return 0 only after real RPC success and both endpoint destroys, otherwise 1.
 * A failed join terminates without unwinding memory that might remain borrowed.
 * RU: Аргументов нет. Владеть endpoint pair, startup synchronization и server thread.
 * Вернуть 0 после настоящих RPC и destroy обоих endpoints, иначе 1.
 * При failed join не освобождать память, которая может оставаться заимствованной.
 */
int main(void)
{
    MemoryStats stats = {0}; /* Outlive client broker, server thread and both adopted endpoints. */
    RemotePhy *client_phy = NULL;
    RemotePhy *server_phy = NULL;
    if (!memory_pair(&client_phy, &server_phy, &stats, 0))
    {
        fprintf(stderr, "memory carrier construction failed\n");
        return 1;
    }
    DemoServer server = {0};
    server.phy = server_phy;
    int error = pthread_mutex_init(&server.lock, NULL);
    if (error)
    {
        fprintf(stderr, "startup mutex: %s\n", strerror(error));
        phyfree(client_phy);
        phyfree(server_phy);
        return 1;
    }
    error = pthread_cond_init(&server.ready, NULL);
    if (error)
    {
        fprintf(stderr, "startup condition: %s\n", strerror(error));
        pthread_mutex_destroy(&server.lock);
        phyfree(client_phy);
        phyfree(server_phy);
        return 1;
    }
    pthread_t thread;
    error = pthread_create(&thread, NULL, memory_server, &server);
    if (error)
    {
        fprintf(stderr, "server creation: %s\n", strerror(error));
        pthread_cond_destroy(&server.ready);
        pthread_mutex_destroy(&server.lock);
        phyfree(client_phy);
        phyfree(server_phy); /* Thread never adopted its grant. */
        return 1;
    }
    server_phy = NULL; /* Successful thread startup transfers that endpoint on every subsequent outcome. */
    pthread_mutex_lock(&server.lock);
    while (!server.initialized)
    {
        pthread_cond_wait(&server.ready, &server.lock); /* Native one-time startup wait, not a guest/cooperative scheduler. */
    }
    int good = server.success;
    pthread_mutex_unlock(&server.lock);
    StyxClient *client = NULL;
    if (good)
    {
        client = styxclientmountphy(client_phy, 256, "demo", "", 3000, 4);
        client_phy = NULL; /* Constructor consumes the grant even if negotiation/attach fails. */
        good = client && !styxclientbidir(client) && use_file(client); /* Ordinary 9P needs no implicit reverse-session adapter. */
    }
    phyfree(client_phy); /* Only still-untransferred startup ownership can remain here. */
    styxclientshutdown(client); /* Interrupt the native broker before its final owner is dropped. */
    styxclientfree(client); /* Joins/free the carrier owner; peer server observes disconnect. */
    if (pthread_join(thread, NULL))
    {
        fprintf(stderr, "server join failed\n");
        _Exit(1);
    }
    good = good && server.clean && stats.destroyed == 2 && stats.read_bytes == stats.write_bytes && stats.read_bytes > 100;
    good = pthread_cond_destroy(&server.ready) == 0 && good;
    good = pthread_mutex_destroy(&server.lock) == 0 && good;
    if (!good)
    {
        fprintf(stderr, "memory PHY RPC/lifetime mismatch: reads=%lu writes=%lu destroyed=%lu\n", stats.read_bytes, stats.write_bytes, stats.destroyed);
        return 1;
    }
    puts("PASS real 9P over memory PHY: readonly status, high64 offsets, fragmented prefixes, no descriptors/TCP/VM");
    return 0;
}
