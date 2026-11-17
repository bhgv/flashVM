/* Copyright (c) 2026 bhgv
 * EN: Host I/O adapters for a job-scoped offer, using existing prepared Styx backing.
 * RU: Host I/O adapters job-scoped offer через existing prepared Styx backing.
 */
#ifndef STYX_MOVIE_OFFER_H
#define STYX_MOVIE_OFFER_H
#include "../../runtime/vfs/launch/movie_offer.h"
#include "announcement.h"
#include "offer_ids.h"
#include "../vfs/file_provider.h"


/* EN/RU: Encode tag0 declaration plus one latest announcement into borrowed output; FVM status.
 * NOT_FOUND means no initial owner yet; no output published on failure. No VM or transport I/O.
 * Native I/O owner calls after source journal readiness, then serializes with other control writes.
 * Кодировать declaration+latest announcement; FVM status, NOT_FOUND до initial bind;
 * после native journal readiness, без VM/transport I/O. */
FVM_Result styxmovieofferframe(DVM_MovieOffer *source, unsigned long tag,
    void *output, size_t capacity, size_t *size);


/* EN/RU: Owned ordinary directory/NULL projecting ctl and movie/<generation> over exact grants.
 * Borrow source/control; control must be a fixed PATH-capable file. Native metadata may block.
 * Root is exportable with existing styxtree/publication; no aname identity change or extra listener.
 * Свой ordinary directory/NULL: ctl и movie/<generation>; exact grants,
 * существующие styxtree/publication, без смены aname/дополнительного listener. */
DVM_Provider *styxmovieofferroot(DVM_MovieOffer *source, DVM_Provider *control);

#endif
