/* Copyright (c) 2026 bhgv
 * EN: Generic terminal-side loader binding for advertisements on an already granted connection.
 * RU: Общая terminal-side привязка загрузчика по объявлениям на уже выданном connection.
 */
#ifndef STYX_MOVIE_RECEIVER_H
#define STYX_MOVIE_RECEIVER_H
#include "client.h"
typedef struct StyxMovieSession StyxMovieSession; /* Owned native receiver and client pin, no VM pointers. */


/* EN/RU: Retain borrowed trusted client; owned session/NULL.
 * No network I/O or implicit authentication; control bytes must belong to this same trusted job/link.
 * Удержать trusted client; свой session/NULL, без I/O/auth. */
StyxMovieSession *styxmoviesession(StyxClient *client);


/* EN/RU: Retain borrowed live session owner;1/0 on NULL/overflow, no I/O.
 * Each successful retain needs free; watcher detaches only after the last public owner.
 * Удержать live session;1/0 при NULL/overflow, каждый retain требует free. */
int styxmoviesessionretain(StyxMovieSession *session);


/* EN/RU: Feed arbitrary control byte fragments;1 accepted/0 protocol failure, sticky closed on failure.
 * Bindings for other types are retained; their data records are ignored. Caller serializes feed.
 * Refuses without mutation while an automatic monitor owns input. Thread-safe against fetch; no VM/SDL/provider I/O. READY generations never move backwards.
 * Принять control fragments;1/0, malformed закрывает session; feed serialized caller,
 * fetch concurrent разрешён, без VM/SDL/provider I/O. */
int styxmoviecontrol(StyxMovieSession *session, const void *bytes, size_t size);


/* EN/RU: Use as FVM_DesktopManagedFetch with session userdata; native blocking, no dial or fallback.
 * Snapshot current offer, Twalk on same connection, consume one owned resource fid.
 * A changed generation or terminal disconnect before completion refuses staged bytes.
 * Использовать как callback; Twalk на том же connection, без dial/fallback;
 * смена generation/disconnect до завершения отклоняет staged bytes. */
FVM_Result styxmovieannouncedfetch(unsigned purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *state, char *resolved, size_t capacity, void *userdata);


/* EN/RU: Copy generation optionally;1 ready,0 pending,-1 retired/invalid, no I/O.
 * Query current local link state as well as native notices; shutdown needs no callback wait.
 * Копировать optional generation;1 ready/0 pending/-1 retired, без I/O. */
int styxmovieavailable(StyxMovieSession *session, unsigned long long *generation);


/* EN/RU: Unexpected control-stream EOF retires this receiver without closing borrowed connection.
 * Неожиданный EOF control stream завершает receiver, не закрывая borrowed connection. */
void styxmoviecontrolend(StyxMovieSession *session);


/* EN/RU: Consume one owner after its feed/fetch callers retire; native watches retire independently.
 * Never shuts down the caller's client. Поглотить receiver после feed/fetch, не закрывая client. */
void styxmoviesessionfree(StyxMovieSession *session);
#endif
