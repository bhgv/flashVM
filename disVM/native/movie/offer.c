/* Copyright (c) 2026 bhgv
 * EN: Scoped latest-state announcement and standard Styx file admission.
 * RU: Scoped latest-state announcement и стандартный Styx file admission.
 */
#include "offer.h"
#include <string.h>


/* EN/RU: See offer.h; stage both complete records before touching caller output.
 * См. offer.h; подготовить оба complete records до изменения caller output. */
FVM_Result styxmovieofferframe(DVM_MovieOffer *source, unsigned long tag,
    void *output, size_t capacity, size_t *size)
{
    DVM_MovieOfferInfo info;
    unsigned char bytes[MOVIE_OFFER_FRAME*2];
    size_t first, second;
    if (!output || !size || !dvm_movie_offer_info(source, &info))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!info.generation)
    {
        return FVM_NOT_FOUND;
    }
    if (!movie_offer_binding(tag, bytes, MOVIE_OFFER_FRAME, &first) ||
        !movie_offer_record(tag, info.closed ? MOVIE_OFFER_CLOSED : MOVIE_OFFER_READY,
            info.generation, bytes+first, sizeof(bytes)-first, &second) || first+second > capacity)
    {
        return FVM_INVALID_ARGUMENT;
    }
    memcpy(output, bytes, first+second);
    *size = first+second;
    return FVM_OK;
}
