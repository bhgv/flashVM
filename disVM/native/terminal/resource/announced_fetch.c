/* Copyright (c) 2026 bhgv
 * EN: Repeated/parallel per-open resource fetches on the exact Unibus client's announced generation.
 * RU: Повторные/параллельные per-open fetch по объявленному generation exact Unibus client.
 */
#include "announced_internal.h"
#include "fetch_scope.h"


/* EN/RU: See announced.h; preserve caller origin until the selected generation passes final admission.
 * См. announced.h; сохранять origin caller до final admission выбранного generation.
 */
FVM_Result styxbidirannouncedfetch(unsigned purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *state, char *resolved, size_t capacity, void *userdata)
{
    StyxBidirMovieReceiver *receiver = userdata; /* Host pins through every concurrent callback. */
    unsigned char request[MOVIE_FRAME]; /* Validation only, no transport emission at this boundary. */
    size_t request_size; /* Exact validated existing request extent. */
    if (!receiver || !write || !resolved || !capacity ||
        (purpose != FVM_DESKTOP_FETCH_CODE && purpose != FVM_DESKTOP_FETCH_DATA) ||
        !movie_request_payload(request, sizeof(request), &request_size,
            purpose == FVM_DESKTOP_FETCH_CODE ? MOVIE_CODE : MOVIE_DATA, reference, base))
    {
        return FVM_INVALID_ARGUMENT;
    }
    MovieFetchScope scope; /* Per-call source; neither caller cancellation nor failure can cancel other fetches. */
    if (!movie_fetch_scope_enter(&scope, receiver->cancel))
    {
        return FVM_ERROR;
    }
    FVM_Result result = FVM_ERROR; /* No caller origin publication before all fences pass. */
    if (movie_receiver_waitcancel(receiver, 0, receiver->timeout, scope.cancel) != 1)
    {
        goto done;
    }
    unsigned long long expected = 0; /* Exact generation selected for this independent fid. */
    if (styxbidirmovieavailable(receiver, &expected) != 1 || dvm_cancel_requested(scope.cancel))
    {
        goto done;
    }
    char origin[MOVIE_PATH]; /* Private until both session and typed-receiver checks pass. */
    size_t bounded = capacity < sizeof(origin) ? capacity : sizeof(origin); /* Honour caller's output bound. */
    result = movie_session_fetch(receiver->session, expected, purpose, reference, base,
        write, state, origin, bounded); /* Existing scoped Twalk/Topen/Twrite/Tread/Tclunk path. */
    if (result == FVM_OK)
    {
        pthread_mutex_lock(&receiver->lock);
        unsigned long long current = 0; /* Final check serialized with typed offer acceptance. */
        if (!dvm_cancel_requested(scope.cancel) && movie_receiver_available(receiver, &current) == 1 && current == expected)
        {
            memcpy(resolved, origin, strlen(origin) + 1);
        }
        else
        {
            result = FVM_ERROR;
        }
        pthread_mutex_unlock(&receiver->lock);
    }
done:
    movie_fetch_scope_leave(&scope);
    return result;
}
