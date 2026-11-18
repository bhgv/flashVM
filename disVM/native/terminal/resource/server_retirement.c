/* Copyright (c) 2026 bhgv
 * EN: Route retirement requests only the resource server's independently pinned native I/O scope.
 * RU: Route retirement отменяет только отдельно удержанный native I/O scope resource server.
 */
#define _POSIX_C_SOURCE 200809L
#include "server_internal.h"


/* EN/RU: Native nonwaiting wake; state owns a cancellation source pin, no server/route/provider pointer.
 * Native wake без ожидания; state удерживает только cancel source, без server/route/provider pointer.
 */
static void retired(void *state)
{
    dvm_cancel_request(state);
}


/* EN/RU: Release independent source pin after admitted callbacks retire, possibly after server disposal.
 * Освободить отдельный source pin после callbacks, возможно после server disposal.
 */
static void release(void *state)
{
    dvm_cancel_release(state);
}


/* EN/RU: See server_internal.h; subscribe before worker publication, replaying already-retired routes.
 * См. server_internal.h; подписаться до worker publication, с replay уже завершённого route.
 */
int movie_server_watch(StyxBidirMovieServer *server)
{
    if (!dvm_cancel_retain(server->cancel))
    {
        return 0;
    }
    server->retired = styxbidirroutewatch(server->route, retired, server->cancel, release);
    if (!server->retired)
    {
        dvm_cancel_release(server->cancel);
        return 0;
    }
    return 1;
}
