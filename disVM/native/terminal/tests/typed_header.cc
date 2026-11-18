/* Copyright (c) 2026 bhgv
 * EN: Link public typed/legacy route APIs from C++98, as used by FlashVM engine adapters.
 * RU: Линковка публичных typed/legacy API маршрута из C++98 для engine-адаптеров FlashVM.
 */
#include "route/types.h"
#include "route/traffic.h"
#include "route/pressure.h"
#include "reply.h"
#include "resource/client.h"
#include "resource/server.h"
#include "resource/announced.h"
#include "resource/outgoing.h"
#include "mount_grant.h"
#include "announcement.h"
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
    if (styxbidirroutequeuemetrics(NULL, 0, 0, NULL) || styxbidirroutepressurelimits(NULL, NULL) ||
        styxbidirroutepressurewait(NULL, 0) || styxbidirroutepressurenext(NULL, NULL) != STYX_CONTROL_END ||
        styxbidirroutequeueinfo(NULL, 0, 0, NULL) || styxbidirrouteservicewait(NULL, 0) ||
        styxbidirrouteservicenext(NULL, NULL, 0, NULL) != STYX_CONTROL_END || styxmountgrantfid(NULL, NULL, 0) || styxbidircontrolwalk(NULL, NULL) || styxbidirroutewalk(NULL, NULL) || styxbidirroutetypednext(NULL, 0, &tag, NULL, 0, &size) != STYX_CONTROL_END ||
        tag != 99 || size != 99 || styxbidirroutetypedwait(NULL, 0, 0) ||
        styxbidirbindingroutes(NULL, NULL, 0, 1, NULL, 0, &schema, 1) ||
        styxbidirroutestate(NULL) != STYX_CONTROL_READ_FAILED ||
        movie_reply_init(NULL, NULL, NULL, NULL, 0) || movie_reply_result(NULL) != FVM_ERROR ||
        movie_reply_accept(NULL, NULL, 0) != MOVIE_REPLY_FAILED ||
        styxbidirmovieclient(NULL, 0, 0, 0) ||
        styxbidirmoviefetch(0, NULL, NULL, NULL, NULL, NULL, 0, NULL) != FVM_INVALID_ARGUMENT ||
        styxbidirroutetypedmatches(NULL, 0, "test", 1) ||
        styxbidirroutetypedwaitcancel(NULL, 0, 0, NULL) ||
        movie_request_payload(NULL, 0, NULL, 0, NULL, NULL) ||
        styxbidircontrolsendwait(NULL, 0, NULL, 0, 0, NULL) ||
        styxbidirroutesendwait(NULL, 0, NULL, 0, 0, NULL) ||
        styxbidirmovieserver(NULL, 0, 0, NULL, 0) || styxbidirmovieserverstate(NULL) != -1 ||
        styxbidirmovieserverwait(NULL, 0) || !styxbidirmovieserverfree(NULL) ||
        styxbidirrouteclient(NULL) || styxbidircontrolclient(NULL) ||
        styxbidirmoviereceiver(NULL, 0, 0) || styxbidirmovieavailable(NULL, NULL) != -1 ||
        styxbidirmoviereceiverwait(NULL, 0, 0) != -1 || styxbidirmoviereceiverstate(NULL) != -1 ||
        styxbidirannouncedfetch(0, NULL, NULL, NULL, NULL, NULL, 0, NULL) != FVM_INVALID_ARGUMENT ||
        !styxbidirmoviereceiverfree(NULL) || styxbidirmoviepublisher(NULL, NULL, 0, 0) ||
        styxbidirmoviepublisherstate(NULL, NULL) != -1 || styxbidirmoviepublisherwait(NULL, 0, 0) != -1 ||
        !styxbidirmoviepublisherfree(NULL) || styxbidircontrolwatch(NULL, NULL, NULL, NULL) ||
        styxbidirroutewatch(NULL, NULL, NULL, NULL) || movie_offer_parse(NULL, 0, NULL, NULL))
    {
        std::fputs("typed route C++98 public ABI failed\n", stderr);
        return 2;
    }
    styxbidirroutefree(NULL);
    styxbidirmovieclientstop(NULL);
    styxbidirmovieclientfree(NULL);
    styxbidirmovieserverstop(NULL);
    styxbidirmoviereceiverstop(NULL);
    styxbidirmoviepublisherstop(NULL);
    return 0;
}
