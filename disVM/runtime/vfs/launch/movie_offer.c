/* Copyright (c) 2026 bhgv
 * EN: Native generation-checked admission and push invalidation for one movie service scope.
 * RU: Native admission с проверкой generation и push invalidation одной movie service scope.
 */
#include "movie_offer.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>


struct DVM_MovieOffer
{
    FVM_PortSync *lock; /* Owns native lifetime/state serialization, never held during callbacks. */
    size_t references; /* Independent owners, including process and host I/O recipient. */
    DVM_Provider *root; /* Owned current endpoint; no namespace or guest pointer serialization. */
    DVM_MovieOfferInfo info; /* Latest coherent state, immutable once closed. */
    DVM_Changes *changes; /* Owned push journal; native observers retain it independently. */
};


/* EN/RU: Notify native observers outside source lock; they inspect latest source state.
 * Уведомить native observers вне source lock; они читают latest source state. */
static void notify(DVM_MovieOffer *source)
{
    changespost(source->changes, DVM_CHANGE_NAMESPACE | DVM_CHANGE_CONNECTION, 0, 0);
}


/* EN/RU: See movie_offer.h; no partial source escapes allocation failure.
 * См. movie_offer.h; partial source не выходит при allocation failure. */
DVM_MovieOffer *dvm_movie_offer_create(void)
{
    DVM_MovieOffer *source = calloc(1, sizeof(*source));
    if (!source)
    {
        return NULL;
    }
    source->lock = fvm_port_sync_create();
    source->changes = dvm_changes_create();
    if (!source->lock || !source->changes)
    {
        if (source->lock)
        {
            fvm_port_sync_destroy(source->lock);
        }
        dvm_changes_destroy(source->changes);
        free(source);
        return NULL;
    }
    source->references = 1;
    return source;
}


/* EN/RU: See movie_offer.h; retain cannot resurrect final storage.
 * См. movie_offer.h; retain не воскрешает final storage. */
int dvm_movie_offer_retain(DVM_MovieOffer *source)
{
    if (!source)
    {
        return 1;
    }
    fvm_port_sync_lock(source->lock);
    int valid = source->references != (size_t)-1;
    if (valid)
    {
        source->references++;
    }
    fvm_port_sync_unlock(source->lock);
    return valid;
}


/* EN/RU: See movie_offer.h; callbacks/destructors run outside the source mutex.
 * См. movie_offer.h; callbacks/destructors вне source mutex. */
void dvm_movie_offer_release(DVM_MovieOffer *source)
{
    if (!source)
    {
        return;
    }
    fvm_port_sync_lock(source->lock);
    int last = --source->references == 0;
    fvm_port_sync_unlock(source->lock);
    if (last)
    {
        dvm_provider_release(source->root);
        dvm_changes_destroy(source->changes);
        fvm_port_sync_destroy(source->lock);
        free(source);
    }
}


/* EN/RU: See movie_offer.h; replacement is compare-and-swap, never an authority lookup by name.
 * См. movie_offer.h; replacement — compare-and-swap, имя не даёт authority. */
uint64_t dvm_movie_offer_bind(DVM_MovieOffer *source, uint64_t expected, DVM_Provider *root)
{
    if (!source || !root || !dvm_provider_retain(root))
    {
        return 0;
    }
    fvm_port_sync_lock(source->lock);
    uint64_t generation = 0;
    DVM_Provider *previous = NULL;
    if (!source->info.closed && source->info.generation == expected && expected < DVM_MOVIE_GENERATION_MAX-1 &&
        (expected || !source->root))
    {
        generation = ++source->info.generation;
        source->info.ready = 1;
        previous = source->root;
        source->root = root;
        root = NULL;
    }
    fvm_port_sync_unlock(source->lock);
    dvm_provider_release(root);
    dvm_provider_release(previous);
    if (generation)
    {
        notify(source);
    }
    return generation;
}


/* EN/RU: See movie_offer.h; a reservation is inaccessible to name lookup and wire encoding.
 * См. movie_offer.h; reservation недоступна для name lookup/wire encoding. */
int dvm_movie_offer_stage(DVM_MovieOffer *source, DVM_Provider *root)
{
    if (!source || !root || !dvm_provider_retain(root))
    {
        return 0;
    }
    fvm_port_sync_lock(source->lock);
    int valid = !source->info.closed && !source->info.generation && !source->root;
    if (valid)
    {
        source->root = root;
        root = NULL;
    }
    fvm_port_sync_unlock(source->lock);
    dvm_provider_release(root);
    return valid;
}


/* EN/RU: See movie_offer.h; exactly one successful commit publishes generation1.
 * См. movie_offer.h; только один successful commit публикует generation1. */
uint64_t dvm_movie_offer_commit(DVM_MovieOffer *source)
{
    if (!source)
    {
        return 0;
    }
    fvm_port_sync_lock(source->lock);
    int valid = !source->info.closed && !source->info.generation && source->root;
    if (valid)
    {
        source->info.generation = 1;
        source->info.ready = 1;
    }
    fvm_port_sync_unlock(source->lock);
    if (valid)
    {
        notify(source);
    }
    return valid ? 1 : 0;
}


/* EN/RU: See movie_offer.h; never revokes a root that has already been published/replaced.
 * См. movie_offer.h; published/replaced root не отзывается через abort reservation. */
int dvm_movie_offer_abort(DVM_MovieOffer *source)
{
    if (!source)
    {
        return 0;
    }
    fvm_port_sync_lock(source->lock);
    int valid = !source->info.closed && !source->info.generation && source->root;
    DVM_Provider *previous = NULL;
    if (valid)
    {
        previous = source->root;
        source->root = NULL;
        source->info.generation = 1;
        source->info.closed = 1;
    }
    fvm_port_sync_unlock(source->lock);
    dvm_provider_release(previous);
    if (valid)
    {
        notify(source);
    }
    return valid;
}


/* EN/RU: See movie_offer.h; close never restores a previous root or permits a new first owner.
 * См. movie_offer.h; close не восстанавливает root и не разрешает нового первого owner. */
int dvm_movie_offer_close(DVM_MovieOffer *source, uint64_t expected)
{
    if (!source)
    {
        return 0;
    }
    fvm_port_sync_lock(source->lock);
    int changed = !source->info.closed && (!expected || source->info.generation == expected);
    DVM_Provider *previous = NULL;
    uint64_t generation = source->info.generation;
    if (changed)
    {
        if (generation < UINT64_MAX)
        {
            generation = ++source->info.generation;
        }
        source->info.ready = 0;
        source->info.closed = 1;
        previous = source->root;
        source->root = NULL;
    }
    fvm_port_sync_unlock(source->lock);
    dvm_provider_release(previous);
    if (changed)
    {
        notify(source);
    }
    return changed;
}


/* EN/RU: See movie_offer.h; latest state is copied without provider or transport callbacks.
 * См. movie_offer.h; latest state копируется без provider/transport callbacks. */
int dvm_movie_offer_info(DVM_MovieOffer *source, DVM_MovieOfferInfo *out)
{
    if (!source || !out)
    {
        return 0;
    }
    fvm_port_sync_lock(source->lock);
    *out = source->info;
    fvm_port_sync_unlock(source->lock);
    return 1;
}


/* EN/RU: See movie_offer.h; canonical representation is exact, not permissive integer parsing.
 * См. movie_offer.h; canonical representation проверяется точно, без permissive parsing. */
int dvm_movie_offer_name(uint64_t generation, char out[DVM_MOVIE_NAME])
{
    if (!generation || !out)
    {
        return 0;
    }
    int size = snprintf(out, DVM_MOVIE_NAME, "movie/%" PRIu64, generation);
    return size > 0 && size < DVM_MOVIE_NAME;
}


/* EN/RU: See movie_offer.h; only the current exact scoped name receives an independent root pin.
 * См. movie_offer.h; только current exact scoped name получает independent root pin. */
DVM_Provider *dvm_movie_offer_acquire(DVM_MovieOffer *source, const char *name)
{
    if (!source || !name)
    {
        return NULL;
    }
    fvm_port_sync_lock(source->lock);
    char expected[DVM_MOVIE_NAME];
    DVM_Provider *root = NULL;
    if (source->info.ready && !source->info.closed &&
        dvm_movie_offer_name(source->info.generation, expected) && !strcmp(name, expected) &&
        dvm_provider_retain(source->root))
    {
        root = source->root;
    }
    fvm_port_sync_unlock(source->lock);
    return root;
}


/* EN/RU: See movie_offer.h; journal pin outlives a native subscription's admitted callbacks.
 * См. movie_offer.h; journal pin живёт до завершения admitted native callbacks. */
DVM_Changes *dvm_movie_offer_observe(DVM_MovieOffer *source)
{
    return source && changesretain(source->changes) ? source->changes : NULL;
}
