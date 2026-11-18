/* Copyright (c) 2026 bhgv
 * EN: Private owned-fid loader and per-RPC native cancellation bridge, no alternate clients.
 * RU: Private owned-fid loader и per-RPC native cancellation bridge, без подмены clients.
 */
#ifndef STYX_MOVIE_FETCH_INTERNAL_H
#define STYX_MOVIE_FETCH_INTERNAL_H
#include "client.h"


/* EN/RU: Consume fid on every outcome; borrow exact client/callback/outputs through native fetch.
 * FVM_OK only after END and final scope check; origin remains private until success.
 * Поглотить fid всегда, exact client/callback/outputs borrowed; origin private до успеха.
 */
FVM_Result movie_fetch_fid(StyxClient *client, StyxFid *fid, unsigned purpose, const char *reference,
    const char *base, FVM_DesktopWrite write, void *state, char *resolved, size_t capacity);


/* EN/RU: One scoped native data RPC with fresh single-use token, writing0 read/1 write; no replay.
 * Inputs borrowed, result initialized on refusal; original precise-prefix acknowledgement preserved.
 * Один scoped native data RPC, fresh token, writing0/1; result готов при отказе, без replay.
 */
void movie_transfer(StyxClient *client, StyxFid *fid, unsigned long long offset, void *bytes,
    size_t size, int writing, StyxTransfer *result);
#endif
