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


/* EN/RU: Accept one already typed/checksummed MOVIE_OFFER_TYPE/VERSION body under exclusive claim.
 * 1 accepted/0 refused; malformed admitted input retires session, no tag rebinding or transport read.
 * Принять checked body объявления под exclusive claim, 1/0; malformed завершает session.
 */
int movie_session_offer(StyxMovieSession *session, const void *bytes, size_t size);


/* EN/RU: Legacy attach-root native fetch with expected generation (0 captures current); no dial/fallback.
 * Reject mismatch before walk and recheck generation/link after bytes; all staging stays caller-owned.
 * Native fetch с expected generation, 0 выбирает current; проверить до walk и после bytes.
 */
FVM_Result movie_session_fetch(StyxMovieSession *session, unsigned long long expected,
    unsigned purpose, const char *reference, const char *base, FVM_DesktopWrite write, void *state,
    char *resolved, size_t capacity);


/* EN/RU: Consume a fid already walked on this session's exact client; shared argument/generation/final fences.
 * Session/arguments borrowed, fid consumed even on refusal; expected0 captures current generation.
 * Поглотить fid exact client данного session, включая отказ; shared argument/generation/final fences.
 */
FVM_Result movie_session_fetch_fid(StyxMovieSession *session, StyxFid *fid, unsigned long long expected,
    unsigned purpose, const char *reference, const char *base, FVM_DesktopWrite write, void *state,
    char *resolved, size_t capacity);
#endif
