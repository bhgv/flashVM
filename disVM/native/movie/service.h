/* Copyright (c) 2026 bhgv
 * EN: Host grants a prepared namespace as a read-only movie acquisition endpoint.
 * RU: Host выдаёт prepared namespace как read-only endpoint загрузки movie.
 */
#ifndef STYX_MOVIE_SERVICE_H
#define STYX_MOVIE_SERVICE_H
#include "provider.h"
#include "namespace.h"


/* EN/RU: Retain borrowed prepared namespace; return owned provider/NULL.
 * Root dot is a duplex S-record file; each rw open owns independent request/response state.
 * Host must supply the authorized per-job namespace; wire names/username grant no rights.
 * Native blocking operations, no VM callbacks or guest suspension; provider outlives opens.
 * Удержать borrowed prepared namespace; свой provider/NULL. Каждый rw open независим.
 * Host задаёт разрешённый namespace задания; wire names/username не дают новых прав. */
DVM_Provider *styxmovieservice(DVM_Namespace *group);
#endif
