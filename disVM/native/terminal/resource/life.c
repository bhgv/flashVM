/* Copyright (c) 2026 bhgv
 * EN: One-shot native resource client ownership and cancellation, independent of the shared route.
 * RU: Владение одноразовым resource client и отменой независимо от общего route.
 */
#include "internal.h"


/* EN/RU: See client.h; validate schema and bounds before allocating native state, no transport I/O.
 * См. client.h; проверить схему и bounds до allocation, без transport I/O.
 */
StyxBidirMovieClient *styxbidirmovieclient(StyxBidirRoute *route, unsigned index,
    unsigned long tag, unsigned timeout)
{
    if (!tag || tag > 0xffffffffUL || !timeout || timeout > 120000 ||
        styxbidirroutelimit(route) < MOVIE_FRAME + 32 ||
        !styxbidirroutetypedmatches(route, index, MOVIE_TYPE, MOVIE_VERSION))
    {
        return NULL;
    }
    StyxBidirMovieClient *client = calloc(1, sizeof(*client)); /* Unpublished native owner. */
    if (!client)
    {
        return NULL;
    }
    if (pthread_mutex_init(&client->lock, NULL))
    {
        free(client);
        return NULL;
    }
    client->cancel = dvm_cancel_create();
    if (!client->cancel)
    {
        pthread_mutex_destroy(&client->lock);
        free(client);
        return NULL;
    }
    client->route = route;
    client->index = index;
    client->tag = tag;
    client->timeout = timeout;
    return client;
}


/* EN/RU: See client.h; no cancellation callback runs under the client mutex.
 * См. client.h; callbacks отмены не исполняются под client mutex.
 */
void styxbidirmovieclientstop(StyxBidirMovieClient *client)
{
    if (client)
    {
        pthread_mutex_lock(&client->lock);
        client->stopped = 1;
        pthread_mutex_unlock(&client->lock);
        dvm_cancel_request(client->cancel);
    }
}


/* EN/RU: See client.h; caller already retired native users, no joins or route ownership change.
 * См. client.h; native callers завершены, без join и изменения владения route.
 */
void styxbidirmovieclientfree(StyxBidirMovieClient *client)
{
    if (client)
    {
        styxbidirmovieclientstop(client);
        dvm_cancel_release(client->cancel);
        pthread_mutex_destroy(&client->lock);
        free(client);
    }
}
