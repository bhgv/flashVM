/* Copyright (c) 2026 bhgv
 * EN: Automatic source outbox drain into complete Unibus S-records with one bounded pair budget.
 * RU: Автоматическое чтение source outbox в полные S-записи Unibus с одним bounded budget пары.
 */
#define _POSIX_C_SOURCE 200809L
#include "outgoing_internal.h"
#include <poll.h>
#include <errno.h>


/* EN/RU: Check one exact existing declaration/offer pair; outputs borrowed and unchanged on failure.
 * Проверить точную existing declaration/offer пару, outputs заимствованы и неизменны при отказе.
 */
static int pair(const void *bytes, size_t size, unsigned long tag, FVM_SRecord *declaration,
    FVM_SRecord *offer, unsigned *state, unsigned long long *generation)
{
    size_t first; /* Complete declaration extent. */
    size_t second; /* Complete offer extent. */
    FVM_SBindings bindings; /* Private checked outgoing association. */
    FVM_SBinding storage; /* Private registry slot. */
    FVM_SBinding binding; /* Copied expected schema. */
    fvm_sbindings_init(&bindings, &storage, 1);
    return fvm_srecord_read(bytes, size, MOVIE_OFFER_FRAME * 2, declaration, &first) == FVM_S_OK &&
        !declaration->tag && first < size &&
        fvm_sbindings_apply(&bindings, bytes, first, &second) == FVM_S_OK && second == first &&
        fvm_sbindings_lookup(&bindings, tag, &binding) == FVM_S_OK && binding.version == MOVIE_OFFER_VERSION &&
        !strcmp(binding.type, MOVIE_OFFER_TYPE) &&
        fvm_srecord_read((const unsigned char *)bytes + first, size - first, MOVIE_OFFER_FRAME * 2, offer, &second) == FVM_S_OK &&
        offer->tag == tag && first + second == size &&
        movie_offer_parse(offer->payload, offer->size, state, generation);
}


/* EN/RU: Admit two whole records under one original budget; never replay an accepted record/prefix.
 * Две целые записи под одним исходным budget, без replay принятых records/prefixes.
 */
static int sending(StyxBidirMoviePublisher *publisher, const FVM_SRecord *declaration, const FVM_SRecord *offer)
{
    struct timespec deadline; /* Entire pair's bounded queue admission deadline. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += publisher->timeout / 1000;
    deadline.tv_nsec += (long)(publisher->timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    const FVM_SRecord *records[] = {declaration, offer}; /* Ordered immutable pair copies. */
    for (unsigned i = 0; i < 2; i++)
    {
        unsigned remaining = movie_budget(&deadline); /* No budget reset between declaration and offer. */
        if (!remaining || dvm_cancel_requested(publisher->cancel) ||
            !styxbidirroutesendwait(publisher->route, records[i]->tag, records[i]->payload,
                records[i]->size, remaining, publisher->cancel))
        {
            return 0;
        }
    }
    return 1;
}


/* EN/RU: See outgoing_internal.h; native poll observes borrowed outbox FD, source callbacks wake it.
 * См. outgoing_internal.h; native poll наблюдает borrowed outbox FD, source callbacks его будят.
 */
void *movie_publisher_run(void *state)
{
    StyxBidirMoviePublisher *publisher = state; /* Host pin through final join. */
    unsigned char bytes[MOVIE_OFFER_FRAME * 2]; /* One complete immutable source snapshot pair. */
    int phase = STYX_MOVIE_PUBLISHER_FAILED; /* Any unexpected source/transport condition is terminal. */
    while (!dvm_cancel_requested(publisher->cancel))
    {
        size_t size = 0; /* Exact outbox record size. */
        int read = styxeventsread(publisher->events, bytes, sizeof(bytes), &size);
        if (read == STYX_EVENT_EMPTY)
        {
            struct pollfd ready = {styxeventsfd(publisher->events), POLLIN, 0}; /* Retained outbox owns descriptor. */
            int result; /* EINTR never becomes a source permission or fake EOF. */
            do
            {
                result = poll(&ready, 1, -1);
            } while (result < 0 && errno == EINTR && !dvm_cancel_requested(publisher->cancel));
            if (result <= 0 || (ready.revents & (POLLNVAL | POLLERR | POLLHUP)))
            {
                break;
            }
            continue;
        }
        if (read != STYX_EVENT_RECORD)
        {
            break;
        }
        FVM_SRecord declaration; /* Borrowed checked frame view. */
        FVM_SRecord offer; /* Borrowed checked frame view. */
        unsigned kind; /* Existing READY/CLOSED state. */
        unsigned long long generation; /* Exact immutable snapshot generation. */
        if (!pair(bytes, size, publisher->tag, &declaration, &offer, &kind, &generation) ||
            !sending(publisher, &declaration, &offer))
        {
            break;
        }
        pthread_mutex_lock(&publisher->lock);
        publisher->generation = generation;
        if (kind == MOVIE_OFFER_CLOSED && publisher->phase == STYX_MOVIE_PUBLISHER_READING)
        {
            publisher->phase = STYX_MOVIE_PUBLISHER_DONE;
        }
        pthread_cond_broadcast(&publisher->changed);
        pthread_mutex_unlock(&publisher->lock);
        if (kind == MOVIE_OFFER_CLOSED)
        {
            phase = STYX_MOVIE_PUBLISHER_DONE;
            break;
        }
    }
    movie_publisher_end(publisher, phase);
    return NULL;
}
