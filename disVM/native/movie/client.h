/* Copyright (c) 2026 bhgv
 * EN: Native Styx callback adapter for the independent FlashVM managed player API.
 * RU: Native Styx callback adapter для независимого FlashVM managed player API.
 */
#ifndef STYX_MOVIE_CLIENT_H
#define STYX_MOVIE_CLIENT_H
#include "../client/client.h"
#include "managed_source.h"


/* EN/RU: Use as FVM_DesktopManagedFetch; userdata is borrowed retained StyxClient.
 * Attached root must be the granted movie service file; each call owns a fresh fid/session.
 * Native blocking, owner thread, serialized by player; no VM entry inside callback.
 * Return FVM_OK only after explicit successful END; all partial bytes remain staged by player.
 * No host file/network fallback; URLs are virtual names interpreted only by granted namespace.
 * Использовать как FVM_DesktopManagedFetch; userdata — borrowed retained StyxClient.
 * Каждый вызов владеет fresh fid, FVM_OK только после END; без local fallback/VM entry. */
FVM_Result styxmoviefetch(unsigned int purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *write_userdata, char *resolved, size_t capacity, void *userdata);
#endif
