/* Copyright (c) 2026 bhgv
 * EN: Nonwaiting per-connection admission, fixed provenance and retained chosen roots.
 * RU: Неблокирующий admission соединения, фиксированный provenance и выбранные корни.
 */
#include "peer_provider.h"
#include "file_provider.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: One connection owner serializes all fields; tickets hold publisher locks.
 * Один owner соединения сериализует поля; tickets используют mutex publisher. */
struct StyxPeer
{
    ExportEndpoint *endpoint; /* Owned submit-only publisher capability. */
    DVM_ExportTicket *ticket; /* Owned outstanding request, or NULL. */
    DVM_Provider *grant; /* Owned chosen provider after approval, or NULL. */
    DVM_ExportRequest request; /* Copied trusted provenance plus logical request. */
};


/* EN/RU: See peer_provider.h; validate all authority inputs before retaining owner.
 * См. peer_provider.h; проверить authority inputs до удержания owner. */
StyxPeer *styxpeer(DVM_Publication *publication, const char *trusted, unsigned flags, unsigned access)
{
    if (!trusted || !*trusted || strlen(trusted) >= DVM_EXPORT_PEER_BYTES ||
        !access || (access & ~FVM_ACCESS_MASK) ||
        (flags & ~(DVM_EXPORT_REMOTE | DVM_EXPORT_AUTHENTICATED)) ||
        ((flags & DVM_EXPORT_REMOTE) && !(flags & DVM_EXPORT_AUTHENTICATED)))
    {
        return NULL;
    }
    StyxPeer *peer = calloc(1, sizeof(*peer));
    if (!peer)
    {
        return NULL;
    }
    peer->endpoint = exportendpoint(publication);
    if (!peer->endpoint)
    {
        free(peer);
        return NULL;
    }
    strcpy(peer->request.peer, trusted);
    peer->request.flags = flags;
    peer->request.access = access;
    return peer;
}


/* EN/RU: Build owned node from selected provider; kind is only a requested hint.
 * Return NULL on metadata/type/allocation failure; grant remains owned by peer.
 * Создать свой node из provider; NULL при ошибке, grant остаётся у peer. */
static StyxBacking *rootnode(StyxPeer *peer)
{
    FVM_FileInfo info;
    if (dvm_provider_info(peer->grant, ".", &info) != FVM_OK ||
        (peer->request.kind == 1 && info.directory) ||
        (peer->request.kind == 2 && !info.directory))
    {
        return NULL;
    }
    return info.directory ? styxtree(peer->grant) : styxbacking(peer->grant);
}


/* EN/RU: See peer.h; pending/granted identity cannot be replaced by wire input.
 * См. peer.h; wire input не заменяет identity pending/granted запроса. */
int styxpeerattach(StyxPeer *peer, const char *name, unsigned kind, StyxBacking **root)
{
    if (!root)
    {
        return -1;
    }
    *root = NULL;
    if (!peer || !name || !*name || strlen(name) >= sizeof(peer->request.name) || kind > 2)
    {
        return -1;
    }
    if ((peer->ticket || peer->grant) &&
        (strcmp(name, peer->request.name) || kind != peer->request.kind))
    {
        return -1;
    }
    if (peer->grant)
    {
        *root = rootnode(peer);
        return *root ? 1 : -1;
    }
    if (!peer->ticket)
    {
        strcpy(peer->request.name, name);
        peer->request.kind = kind;
        peer->ticket = exportrequest(peer->endpoint, &peer->request);
        if (!peer->ticket)
        {
            return -1;
        }
    }
    int status = dvm_export_take(peer->ticket, &peer->grant);
    if (!status)
    {
        return 0;
    }
    dvm_export_release(peer->ticket);
    peer->ticket = NULL;
    if (status != 1)
    {
        return -1;
    }
    *root = rootnode(peer);
    if (!*root)
    {
        dvm_provider_release(peer->grant);
        peer->grant = NULL;
        return -1;
    }
    return 1;
}


/* EN/RU: See peer.h; cancellation precedes endpoint release, no managed execution.
 * См. peer.h; отмена до release endpoint, без managed execution. */
void styxpeerfree(StyxPeer *peer)
{
    if (peer)
    {
        dvm_export_release(peer->ticket);
        dvm_provider_release(peer->grant);
        exportendpointfree(peer->endpoint);
        free(peer);
    }
}
