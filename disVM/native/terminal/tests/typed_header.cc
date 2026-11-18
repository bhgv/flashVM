/* Copyright (c) 2026 bhgv
 * EN: Link public typed/legacy route APIs from C++98, as used by FlashVM engine adapters.
 * RU: Линковка публичных typed/legacy API маршрута из C++98 для engine-адаптеров FlashVM.
 */
#include "route/types.h"
#include "reply.h"
#include "resource/client.h"
#include "request_payload.h"
#include <cstdio>


/* EN/RU: Exercise null/invalid public entry points without transport or native workers; 0 success.
 * Проверить null/invalid публичные вызовы без транспорта и native workers, 0 — успех.
 */
int main()
{
    unsigned long tag = 99; /* Output sentinel for invalid reader. */
    size_t size = 99; /* Output sentinel for invalid reader. */
    StyxRouteType schema = {"test.file", 1}; /* Borrowed preflight input. */
    if (styxbidirroutetypednext(NULL, 0, &tag, NULL, 0, &size) != STYX_CONTROL_END ||
        tag != 99 || size != 99 || styxbidirroutetypedwait(NULL, 0, 0) ||
        styxbidirbindingroutes(NULL, NULL, 0, 1, NULL, 0, &schema, 1) ||
        styxbidirroutestate(NULL) != STYX_CONTROL_READ_FAILED ||
        movie_reply_init(NULL, NULL, NULL, NULL, 0) || movie_reply_result(NULL) != FVM_ERROR ||
        movie_reply_accept(NULL, NULL, 0) != MOVIE_REPLY_FAILED ||
        styxbidirmovieclient(NULL, 0, 0, 0) ||
        styxbidirmoviefetch(0, NULL, NULL, NULL, NULL, NULL, 0, NULL) != FVM_INVALID_ARGUMENT ||
        styxbidirroutetypedmatches(NULL, 0, "test", 1) ||
        styxbidirroutetypedwaitcancel(NULL, 0, 0, NULL) ||
        movie_request_payload(NULL, 0, NULL, 0, NULL, NULL))
    {
        std::fputs("typed route C++98 public ABI failed\n", stderr);
        return 2;
    }
    styxbidirroutefree(NULL);
    styxbidirmovieclientstop(NULL);
    styxbidirmovieclientfree(NULL);
    return 0;
}
