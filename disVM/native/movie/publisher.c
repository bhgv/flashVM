/* Copyright (c) 2026 bhgv
 * EN: Adapt scoped offer notices into native pollable, bounded announcement records.
 * RU: Адаптировать scoped offer notices в native pollable bounded announcement records.
 */
#include "publisher.h"
#include "../vfs/events_provider.h"
#include <stdlib.h>


/* EN/RU: Independent immutable encoder context lives through in-flight native notices.
 * Independent immutable encoder context живёт до завершения native notices. */
typedef struct Publication
{
    DVM_MovieOffer *source; /* Owned trusted admission state, never a guest pointer. */
    unsigned long tag; /* Host-reserved tag on this connection's outgoing direction. */
} Publication;


/* EN/RU: Admit only a published/retired source; state/event borrowed,1 admits,0 drops.
 * Native snapshot only; no I/O, queue reentry or managed execution on the publisher.
 * Допустить published/retired source; borrowed state/event,1/0, без I/O/reentry/VM. */
static int allow(void *state, const StyxEvent *event)
{
    Publication *publication = state;
    DVM_MovieOfferInfo current;
    (void)event;
    return dvm_movie_offer_info(publication->source, &current) && current.generation != 0;
}


/* EN/RU: Encode latest coherent state on the serialized reader, not on a source process.
 * Borrow state/event/output through return;1 complete pair,0 preserves the queue head.
 * Latest-state duplicates after coalesced changes are harmless to the generic receiver.
 * Кодировать latest state на serialized reader;1 complete pair/0 сохраняет head;
 * state/event/output borrowed, duplicates допустимы receiver. */
static int encode(void *state, const StyxEvent *event, void *bytes, size_t capacity, size_t *size)
{
    Publication *publication = state;
    (void)event;
    return styxmovieofferframe(publication->source, publication->tag, bytes, capacity, size) == FVM_OK;
}


/* EN/RU: Consume encoder/source pin after all admitted callbacks/readers retire; no joins.
 * Поглотить encoder/source pin после callbacks/readers, без join. */
static void release(void *state)
{
    Publication *publication = state;
    dvm_movie_offer_release(publication->source);
    free(publication);
}


/* EN/RU: See publisher.h; underlying event source handles initial-edge races and bounded overflow.
 * См. publisher.h; event source закрывает initial-edge race и bounded overflow. */
StyxEvents *styxmovieofferwatch(DVM_MovieOffer *source, unsigned long tag)
{
    unsigned char probe[MOVIE_OFFER_FRAME];
    size_t size;
    if (!source || !movie_offer_binding(tag, probe, sizeof(probe), &size))
    {
        return NULL;
    }
    Publication *publication = calloc(1, sizeof(*publication));
    if (!publication)
    {
        return NULL;
    }
    if (!dvm_movie_offer_retain(source))
    {
        free(publication);
        return NULL;
    }
    publication->source = source;
    publication->tag = tag;
    DVM_Changes *journal = dvm_movie_offer_observe(source);
    StyxEventPolicy policy = {
        STYX_EVENT_NAMESPACE | STYX_EVENT_CONNECTION, 0, MOVIE_OFFER_FRAME*2,
        allow, encode, publication, release
    };
    StyxEvents *outbox = journal ? styxeventsource(journal, &policy) : NULL;
    dvm_changes_destroy(journal);
    if (!outbox)
    {
        release(publication);
    }
    return outbox;
}
