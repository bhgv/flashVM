/* Copyright (c) 2026 bhgv
 * EN: Deterministic native notice-delay injection; shutdown visibility must not depend on delivery.
 * RU: Детерминированная native notice-delay injection; shutdown виден независимо от delivery.
 */
#include "receiver.h"
#include <stdlib.h>
#include <string.h>
StyxWatch *__real_styxclientwatch(StyxClient *, StyxTerminal, void *, void (*)(void *));


/* EN/RU: Own original callback state under the real native watch, without forwarding its notice.
 * Владеть original callback state под настоящим native watch без передачи notice. */
typedef struct Delayed
{
    void *state; /* Original receiver-owned state adopted by real watch on success. */
    void (*release)(void *); /* Original cleanup after callbacks retire. */
} Delayed;


/* EN/RU: Simulate a terminal notice still queued; borrowed state/reason, no wait or I/O.
 * Имитировать ещё не доставленный terminal notice; borrowed state/reason, без wait/I/O. */
static void delayed(void *state, int reason)
{
    (void)state;
    (void)reason;
}


/* EN/RU: Consume adopted original state and wrapper after all admitted real callbacks retire.
 * Поглотить original state и wrapper после завершения всех real callbacks. */
static void released(void *state)
{
    Delayed *notice = state;
    if (notice->release)
    {
        notice->release(notice->state);
    }
    free(notice);
}


/* EN/RU: Suppress only the disconnect fixture's receiver notice; production library stays unchanged.
 * Real watch retains native lifecycle; allocation/registration failure leaves state caller-owned.
 * Подавить receiver notice только disconnect fixture, сохраняя real watch lifetime; NULL не принимает state. */
StyxWatch *__wrap_styxclientwatch(StyxClient *client, StyxTerminal notify, void *state, void (*release)(void *))
{
    const char *mode = getenv("MOVIE_TEST_ANNOUNCE");
    if (!mode || strcmp(mode, "disconnect"))
    {
        return __real_styxclientwatch(client, notify, state, release);
    }
    Delayed *notice = malloc(sizeof(*notice));
    if (!notice)
    {
        return NULL;
    }
    notice->state = state;
    notice->release = release;
    StyxWatch *watch = __real_styxclientwatch(client, delayed, notice, released);
    if (!watch)
    {
        free(notice);
    }
    return watch;
}
