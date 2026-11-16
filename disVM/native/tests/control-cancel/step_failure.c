/* Copyright (c) 2026 bhgv
 * EN: Private second-fragment allocation/cancellation controls over the production walker.
 * RU: Private second-fragment allocation/cancellation controls над production walker.
 */
#include "../../client/internal.h"
static unsigned steps; /* Actual cancellation-fragment creation attempts. */


/* EN/RU: At the second logical fragment only, CONTROL1 refuses setup,2 requests cancellation.
 * Borrow outer token, return its production child or NULL; no wire/provider/guest bypass.
 * Только во втором logical fragment CONTROL1 отклоняет setup,2 запрашивает cancellation. */
static StyxCancel *controlledstep(StyxCancel *cancel);
#define styxcancelstep controlledstep
#include "../../client/walk.c"
#undef styxcancelstep


/* EN/RU: See prototype; all unmodified fragment setup uses the real native implementation.
 * См. prototype; неизменённый fragment setup использует real native implementation. */
static StyxCancel *controlledstep(StyxCancel *cancel)
{
    steps++;
    if (steps == 2)
    {
        if (CONTROL == 1)
        {
            return NULL;
        }
        styxcancelrequest(cancel);
    }
    return styxcancelstep(cancel);
}
