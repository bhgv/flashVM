/* Copyright (c) 2026 bhgv
 * EN: Private ownership bridge between movie receiver and its one autonomous native feeder.
 * RU: Private ownership bridge между movie receiver и одним autonomous native feeder.
 */
#ifndef STYX_MOVIE_SESSION_INTERNAL_H
#define STYX_MOVIE_SESSION_INTERNAL_H
#include "receiver.h"


/* EN/RU: Acquire/release exclusive feed claim on owned session;1/0, no I/O.
 * Acquire refuses closed, partial manual input or another monitor; release only on setup failure
 * or after the worker retires. Claim does not retain; caller holds a session owner.
 * Acquire/release feed claim;1/0 без I/O, отказ при closed/partial input/другом monitor. */
int movie_session_claim(StyxMovieSession *session, int acquire);


/* EN/RU: Borrow immutable client through retained session; no I/O or ownership transfer.
 * Заимствовать immutable client под retained session, без I/O/передачи ownership. */
StyxClient *movie_session_client(StyxMovieSession *session);


/* EN/RU: Feed borrowed fragments under exclusive monitor claim;1/0, same protocol/lock as manual feed.
 * Принять borrowed fragments под exclusive claim;1/0, общий protocol/lock с manual feed. */
int movie_session_feed(StyxMovieSession *session, const void *bytes, size_t size);
#endif
